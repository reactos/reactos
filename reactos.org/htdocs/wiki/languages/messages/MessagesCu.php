<?php
/** Church Slavic (Словѣ́ньскъ / ⰔⰎⰑⰂⰡⰐⰠⰔⰍⰟ)
 *
 * @ingroup Language
 * @file
 *
 * @author Svetko
 * @author Wolliger Mensch
 * @author ОйЛ
 */

$separatorTransformTable = array(
	',' => ".",
	'.' => ','
);

$linkPrefixExtension = true;

$namespaceNames = array(
	NS_MEDIA          => 'Срѣ́дьства',
	NS_SPECIAL        => 'Наро́чьна',
	NS_MAIN           => '',
	NS_TALK           => 'Бєсѣ́да',
	NS_USER           => 'По́льꙃєватєл҄ь',
	NS_USER_TALK      => 'По́льꙃєватєлꙗ_бєсѣ́да',
	# NS_PROJECT set by \$wgMetaNamespace
	NS_PROJECT_TALK   => '{{grammar:genitive|$1}}_бєсѣ́да',
	NS_IMAGE          => 'Ви́дъ',
	NS_IMAGE_TALK     => 'Ви́да_бєсѣ́да',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'MediaWiki_бєсѣ́да',
	NS_TEMPLATE       => 'Обраꙁь́ць',
	NS_TEMPLATE_TALK  => 'Обраꙁьца́_бєсѣ́да',
	NS_HELP           => 'По́мощь',
	NS_HELP_TALK      => 'По́мощи_бєсѣ́да',
	NS_CATEGORY       => 'Катигорі́ꙗ',
	NS_CATEGORY_TALK  => 'Катигорі́ѩ_бєсѣ́да',
);

$namespaceAliases = array(
	'Срѣдьства'                      => NS_MEDIA,
	'Нарочьна'                       => NS_SPECIAL,
	'Бесѣда'                         => NS_TALK,
	'Польѕевател҄ь'                  => NS_USER,
	'Польѕевател_бесѣда'             => NS_USER_TALK,
	'{{grammar:genitive|$1}}_бесѣда' => NS_PROJECT_TALK,
	'Видъ'                           => NS_IMAGE,
	'Вида_бесѣда'                    => NS_IMAGE_TALK,
	'MediaWiki_бесѣда'               => NS_MEDIAWIKI_TALK,
	'Образьць'                       => NS_TEMPLATE,
	'Образьца_бесѣда'                => NS_TEMPLATE_TALK,
	'Помощь'                         => NS_HELP,
	'Помощи_бесѣда'                  => NS_HELP_TALK,
	'Катигорї'                      => NS_CATEGORY,
	'Катигорїѩ_бесѣда'               => NS_CATEGORY_TALK,
);

$defaultDateFormat = 'mdy';

$dateFormats = array(
	'mdy time' => 'H:i',
	'mdy date' => 'xg j числа, Y',
	'mdy both' => 'H:i, xg j числа, Y',

	'dmy time' => 'H:i',
	'dmy date' => 'j F Y',
	'dmy both' => 'H:i, j F Y',

	'ymd time' => 'H:i',
	'ymd date' => 'Y F j',
	'ymd both' => 'H:i, Y F j',

	'ISO 8601 time' => 'xnH:xni:xns',
	'ISO 8601 date' => 'xnY-xnm-xnd',
	'ISO 8601 both' => 'xnY-xnm-xnd"T"xnH:xni:xns',
);

$linkTrail = '/^([a-zабвгдеєжѕзїіıићклмнопсстѹфхѡѿцчшщъыьѣюѥѧѩѫѭѯѱѳѷѵґѓђёјйљњќуўџэ҄я“»]+)(.*)$/sDu';

$messages = array(
# Dates
'sunday'        => 'нєдѣ́лꙗ',
'monday'        => 'понедѣл҄ьникъ',
'tuesday'       => 'въторьникъ',
'wednesday'     => 'срѣда',
'thursday'      => 'четврьтъкъ',
'friday'        => 'пѧтъкъ',
'saturday'      => 'сѫбота',
'sun'           => 'н҃д',
'mon'           => 'п҃н',
'tue'           => 'в҃т',
'wed'           => 'с҃р',
'thu'           => 'ч҃т',
'fri'           => 'п҃т',
'sat'           => 'с҃б',
'january'       => 'їаноуа́рїи',
'february'      => 'февроуа́рїи',
'march'         => 'мартїи',
'april'         => 'апрі́лїи',
'may_long'      => 'ма́їи',
'june'          => 'їоу́нїи',
'july'          => 'їоу́лїи',
'august'        => 'аѵ́гоустъ',
'september'     => 'сєптє́мврїи',
'october'       => 'октѡ́врїи',
'november'      => 'ноє́мврїи',
'december'      => 'дєкє́мврїи',
'january-gen'   => 'їаноуа́рїꙗ',
'february-gen'  => 'фєвроуа́рїꙗ',
'march-gen'     => 'ма́ртїꙗ',
'april-gen'     => 'апрі́лїꙗ',
'may-gen'       => 'ма́їꙗ',
'june-gen'      => 'їоу́нїꙗ',
'july-gen'      => 'їоу́лїꙗ',
'august-gen'    => 'аѵ́гоуста',
'september-gen' => 'сєптє́мврїꙗ',
'october-gen'   => 'октѡ́врїꙗ',
'november-gen'  => 'ноє́мврїꙗ',
'december-gen'  => 'дєкє́мврїꙗ',
'jan'           => 'ꙗ҃н',
'feb'           => 'фє҃в',
'mar'           => 'ма҃р',
'apr'           => 'ап҃р',
'may'           => 'маи',
'jun'           => 'їо҃ун',
'jul'           => 'їо҃ул',
'aug'           => 'аѵ҃г',
'sep'           => 'сє҃п',
'oct'           => 'ок҃т',
'nov'           => 'но҃є',
'dec'           => 'дє҃к',

# Categories related messages
'pagecategories'  => '{{PLURAL:$1|Катигорі́ꙗ|Катигорі́и|Катигорі́ѩ|Катигорі́ѩ}}',
'category_header' => 'катигорі́ѩ ⁖ $1 ⁖ страни́цѧ',

'linkprefix' => '/^(.*?)(„|«)$/sD',

'about'          => 'опьса́ниѥ',
'qbedit'         => 'испра́ви',
'qbpageoptions'  => 'си страни́ца',
'qbmyoptions'    => 'моꙗ́ страни́цѧ',
'qbspecialpages' => 'наро́чьнꙑ страни́цѧ',
'mypage'         => 'моꙗ́ страни́ца',
'mytalk'         => 'моꙗ́ бєсѣ́да',
'navigation'     => 'пла́ваниѥ',
'and'            => 'и',

'errorpagetitle'   => 'блаꙁна',
'help'             => 'по́мощь',
'search'           => 'иска́ниѥ',
'searchbutton'     => 'ищи́',
'go'               => 'прѣиди́',
'searcharticle'    => 'прѣиди́',
'history'          => 'страни́цѧ їсторі́ꙗ',
'history_short'    => 'їсторі́ꙗ',
'printableversion' => 'пєча́тьнъ о́браꙁъ',
'permalink'        => 'въи́ньна съвѧ́ꙁь',
'edit'             => 'испра́ви',
'create'           => 'сътворѥ́ниѥ',
'editthispage'     => 'си страни́цѧ исправлѥ́ниѥ',
'delete'           => 'поничьжє́ниѥ',
'deletethispage'   => 'си страни́цѧ поничьжє́ниѥ',
'protect'          => 'ꙁабранѥ́ниѥ',
'protectthispage'  => 'си страни́цѧ ꙁабранє́ниѥ',
'unprotect'        => 'поущє́ниѥ',
'newpage'          => 'но́ва страни́ца',
'talkpage'         => 'си страни́цѧ бєсѣ́да',
'talkpagelinktext' => 'бєсѣ́да',
'specialpage'      => 'наро́чьна страни́ца',
'talk'             => 'бєсѣ́да',
'toolbox'          => 'орѫ́диꙗ',
'otherlanguages'   => 'ДРОУГꙐ́ ѨꙀꙐКꙐ́',
'redirectedfrom'   => '(прѣнаправлѥ́ниѥ о́тъ ⁖ $1 ⁖)',
'redirectpagesub'  => 'прѣнаправлѥ́ниѥ',
'lastmodifiedat'   => 'страни́цѧ послѣ́дьнꙗ мѣ́на сътворѥна́ $2 · $1 бѣ ⁙', # $1 date, $2 time
'jumptonavigation' => 'пла́ваниѥ',
'jumptosearch'     => 'иска́ниѥ',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'О {{grammar:instrumental|{{SITENAME}}}}',
'aboutpage'            => 'Project:О сѥ́мь опꙑтьствова́нии',
'copyright'            => 'по́дъ прощє́ниѥмь $1 пьса́но ѥ́стъ',
'currentevents'        => 'сѫ́щѧѩ вѣ́щи',
'currentevents-url'    => 'Project:Сѫ́щѧѩ вѣ́щи',
'edithelppage'         => 'Help:Исправлѥ́ниѥ страни́цѧ',
'mainpage'             => 'гла́вьна страни́ца',
'mainpage-description' => 'гла́вьна страни́ца',
'portal'               => 'обьщє́ниꙗ съвѣ́тъ',
'portal-url'           => 'Project:Обьщє́ниꙗ съвѣ́тъ',

'newmessageslink'     => 'но́ви напь́саниꙗ',
'newmessagesdifflink' => 'послѣ́дьнꙗ мѣ́на',
'editsection'         => 'испра́ви',
'editold'             => 'испра́ви',
'hidetoc'             => 'съкрꙑи',
'viewdeleted'         => '$1 ви́дєти хо́щєши ;',
'red-link-title'      => '$1 (ѥщє нє напь́сано ѥ́стъ)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'члѣ́нъ',
'nstab-user'      => 'по́льꙃєватєл҄ь',
'nstab-media'     => 'срѣ́дьства',
'nstab-special'   => 'наро́чьна',
'nstab-project'   => 'съвѣ́тъ',
'nstab-image'     => 'ви́дъ',
'nstab-mediawiki' => 'напьса́ниѥ',
'nstab-template'  => 'обраꙁь́ць',
'nstab-help'      => 'страни́ца по́мощи',
'nstab-category'  => 'катигорі́ꙗ',

# General errors
'viewsource' => 'страни́цѧ исто́чьнъ о́браꙁъ',

# Login and logout pages
'logouttitle'             => 'ис̾хо́дъ',
'loginpagetitle'          => 'Въходъ',
'yourname'                => 'твоѥ́ и́мѧ',
'yourpassword'            => 'Таино слово напиши',
'yourpasswordagain'       => 'Опакы таиноѥ слово напиши',
'login'                   => 'Въниди',
'nav-login-createaccount' => 'въниди / съзи́жди си мѣ́сто',
'userlogin'               => 'въниди / съзи́жди си мѣ́сто',
'logout'                  => 'ис̾хо́дъ',
'userlogout'              => 'ис̾хо́дъ',
'createaccount'           => 'Cъзижди си мѣсто',
'gotaccount'              => 'Мѣсто ти ѥстъ ли? $1.',
'gotaccountlink'          => 'Въниди',
'userexists'              => 'сѫщє по́льꙃєватєлꙗ и́мѧ пьса ⁙ ино иꙁобрѧщи',
'username'                => 'по́льꙃєватєлꙗ и́мѧ :',
'uid'                     => 'по́льꙃєватєлꙗ число́ :',
'yourlanguage'            => 'ѩꙁꙑ́къ :',
'yournick'                => 'аѵто́графъ :',
'loginerror'              => 'Въхода блазна',
'accountcreated'          => 'мѣ́сто сътворєно́ ѥ́стъ',
'loginlanguagelabel'      => 'ѩꙁꙑ́къ : $1',

# Edit page toolbar
'link_sample'    => 'съвѧ́ꙁи и́мѧ',
'extlink_sample' => 'http://www.example.com съвѧ́ꙁи и́мѧ',

# Edit pages
'summary'            => 'опьса́ниѥ',
'minoredit'          => 'ма́лаꙗ мѣ́на',
'watchthis'          => 'си страни́цѧ блюдє́ниѥ',
'savearticle'        => 'съхранѥ́ниѥ',
'loginreqlink'       => 'Въниди',
'newarticle'         => '(но́въ)',
'clearyourcache'     => '<big>НАРОЧИ́ТО:</big> По съхранѥ́нии мо́жєши обити́ своѥго́ съмотри́ла съхра́нъ да ви́дѣлъ би мѣ́нꙑ ⁙ Mozilla ли Firefox ли Safari ли жьмꙑ́и Shift а мꙑ́шиѭ жьми́ Reload и́ли жьми́ Ctrl-Shift-R (Cmd-Shift-R вън Apple Mac)  ⁙ Konqueror ли жьми́ кро́мѣ Reload и́ли F5 ⁙ О́пєрꙑ по́льꙃєватєльмъ мо́жєть бꙑ́ти ноужда́ пльнѣ пони́чьжити и́хъ съмотри́ла съхра́нъ въ Tools > Preferences ⁙ IE ли жьмꙑ́и Ctrl а мꙑ́шиѭ жьми́ Refresh и́ли жьми́ Ctrl-F5',
'note'               => '<strong>НАРОЧИ́ТО:</strong>',
'editing'            => 'исправлѥ́ниѥ: $1',
'editingsection'     => 'исправлѥ́ниѥ ⁖ $1 ⁖ (чѧ́сть)',
'templatesused'      => 'сѥѩ страни́цѧ с҄и обраꙁьци́ по́льꙃоуѭтъ сѧ сѫ́тъ :',
'template-protected' => '(ꙁабранєно ѥ́стъ)',

# History pages
'viewpagelogs' => 'си страни́цѧ їсторі́ѩ',
'cur'          => 'нꙑ҃н',
'last'         => 'пс҃лд',
'historyempty' => '(поу́сто)',

# Revision feed
'history-feed-title' => 'мѣ́нъ їсторі́ꙗ',

# Revision deletion
'revdelete-uname' => 'по́льꙃєватєлꙗ и́мѧ',

# Search results
'searchresults'            => 'иска́ниꙗ ито́гъ',
'search-result-size'       => '$1 ({{PLURAL:$2|$2 сло́во|$2 сло́ва|$2 словє́съ}})',
'search-interwiki-caption' => 'ро́дьствьна опꙑтьствова́ниꙗ',
'searchall'                => 'вьсꙗ́',
'powersearch'              => 'ищи́',

# Preferences page
'preferences'         => 'строи',
'mypreferences'       => 'мои строи',
'prefs-rc'            => 'послѣ́дьнѩ мѣ́нꙑ',
'prefs-watchlist'     => 'блюдє́ниꙗ',
'searchresultshead'   => 'иска́ниѥ',
'prefs-searchoptions' => 'иска́ниꙗ строи́',
'files'               => 'дѣла́',

# Groups
'group-user'       => 'по́льꙃєватєлє',
'group-bot'        => 'аѵтома́ти',
'group-sysop'      => 'съмотри́тєлє',
'group-bureaucrat' => 'Чинода́тєлє',

'group-user-member'       => 'по́льꙃєватєл҄ь',
'group-bot-member'        => 'аѵтома́тъ',
'group-sysop-member'      => 'съмотри́тєл҄ь',
'group-bureaucrat-member' => 'чинода́тєл҄ь',

'grouppage-user'       => '{{ns:project}}:По́льꙃєватєлє',
'grouppage-bot'        => '{{ns:project}}:Аѵтома́ти',
'grouppage-sysop'      => '{{ns:project}}:Съмотри́тєлє',
'grouppage-bureaucrat' => '{{ns:project}}:Чинода́тєлє',

# User rights log
'rightslog' => 'чинода́тєльства їсторі́ꙗ',

# Recent changes
'nchanges'        => '$1 {{PLURAL:$1|мѣ́на|мѣ́нꙑ|мѣ́нъ}}',
'recentchanges'   => 'послѣ́дьнѩ мѣ́нꙑ',
'rcshowhideminor' => '$1 ма́лꙑ мѣ́нꙑ',
'rcshowhidebots'  => '$1 аѵтома́тъ',
'rcshowhidemine'  => '$1 моꙗ́ мѣ́нꙑ',
'diff'            => 'ра҃ꙁн',
'hist'            => 'їс҃т',
'hide'            => 'съкрꙑи',
'minoreditletter' => 'м҃л',
'newpageletter'   => 'н҃в',
'boteditletter'   => 'а҃ѵ',

# Recent changes linked
'recentchangeslinked'      => 'съвѧ́ꙁанꙑ страни́цѧ',
'recentchangeslinked-page' => 'страни́цѧ и́мѧ :',

# Upload
'upload'           => 'положє́ниѥ дѣ́ла',
'uploadbtn'        => 'положє́ниѥ дѣ́ла',
'uploadlog'        => 'дѣ́лъ положє́ниꙗ їсторі́ꙗ',
'uploadlogpage'    => 'дѣ́лъ положє́ниꙗ їсторі́ꙗ',
'successfulupload' => 'дѣ́ло положєно ѥ́стъ',
'uploadedimage'    => '⁖ [[$1]] ⁖ положє́нъ ѥ́стъ',
'watchthisupload'  => 'си страни́цѧ блюдє́ниѥ',

# Special:ImageList
'imgfile'        => 'дѣ́ло',
'imagelist'      => 'дѣ́лъ ката́логъ',
'imagelist_name' => 'и́мѧ',
'imagelist_user' => 'по́льꙃєватєл҄ь',
'imagelist_size' => 'мѣ́ра',

# Image description page
'filehist-deleteone' => 'поничьжє́ниѥ',
'filehist-current'   => 'нꙑнѣщьн҄ь о́браꙁъ',
'filehist-user'      => 'по́льꙃєватєл҄ь',
'imagelinks'         => 'съвѧ́ꙁи',

# File deletion
'filedelete-submit' => 'поничьжє́ниѥ',

# MIME search
'mimetype' => 'MIME тѵ́пъ :',
'download' => 'поѩ́ти',

# Random page
'randompage' => 'страни́ца въ нєꙁаа́пѫ',

# Statistics
'statistics'    => 'Статїстїка',
'sitestats'     => '{{SITENAME}} статїстїка',
'userstats'     => 'Польѕевателъ статїстїка',
'sitestatstext' => "Сьдє '''$1''' {{PLURAL:$1|страни́ца ѥ́стъ|страни́ци ѥ́стє|страни́цѧ сѫ́тъ|страни́ць сѫ́тъ}} · посрѣдѣ {{PLURAL:$1|ѩже|ѥюжє|ихъжє|ихъжє}} и бєсѣдꙑ · и страницѧ о {{SITENAME}} · и ꙃѣло малꙑ члѣ́ни · и прѣнаправлѥниꙗ · и дроугꙑ́ страницѧ сѫ́тъ · ѩжє истиньнꙑ члѣ́ни нє сѫ́тъ ⁙ Бєжихъ Википєдїи '''$2''' {{PLURAL:$2|страни́ца ѥ́стъ|страни́ци ѥ́стє|страни́цѧ сѫ́тъ|страни́ць сѫ́тъ}} ѩжє {{PLURAL:$2|истиньна члѣ́нъ ѥстъ|истиньнѣ члѣ́на ѥ́стє|истиньнꙑ члѣ́ни сѫ́тъ|истиньнꙑ члѣ́ни сѫ́тъ}}

Такождє '''$8''' {{PLURAL:$8|дѣло положєно ѥстъ|дѣлѣ положєно ѥстє|дѣла положєно сѫ́тъ|дѣлъ положєно сѫ́тъ}}

О прьваѥго {{grammar:genitive|{{SITENAME}}}} дьнє '''$4''' {{PLURAL:$4|исправлѥ́ниѥ сътворѥно ѥ́стъ|исправлѥ́нии сътворѥнѣ ѥ́стє|исправлѥ́ниꙗ сътворѥно сѫ́тъ|исправлѥ́нии сътворѥно сѫ́тъ}} ⁙ Сѥ значитъ ꙗко кажьдо страница '''$5''' исправлѥниꙗ иматъ · а къжьдо мѣ́на '''$6''' {{PLURAL:$6|раꙁъ съмощрѥна бѣ|раꙁа съмощрѥна бѣашєтє|раꙁъ съмощрѥна бѣ|раꙁъ съмощрѥна бѣашѧ}}  

[http://www.mediawiki.org/wiki/Manual:Job_queue Дѣ́иствъ чрѣ́дꙑ] дльгота '''$7''' ѥ́стъ",
'userstatstext' => "С҄ьдє $1 [[Special:ListUsers|{{plural:$1|по́льꙃєватєл҄ь|по́льꙃеватєлꙗ|п́ольꙃєватєлє|по́льꙃєватєлъ}}]] {{plural:$1|ѥ́стъ|ѥ́стє|сѫ́тъ|сѫ́тъ}} · {{plural:$1|ижажє|ижєюжє|ижихъжє|ижихъжє}} '''$2''' (или '''$4%''') {{plural:$5|[[Project:Съмотри́тєлє|{{plural:$2|съмотри́тєл҄ь|съмотри́тєлє|съмотри́тєлє|съмотри́тєлъ}}]]}} {{plural:$2|ѥ́стъ|ѥ́стє|сѫ́тъ|сѫ́тъ}}",

'disambiguations'     => 'мъногосъмꙑ́слиꙗ',
'disambiguationspage' => 'Template:мъногосъмꙑ́слиѥ',

'brokenredirects-edit'   => '(испра́ви)',
'brokenredirects-delete' => '(поничьжє́ниѥ)',

# Miscellaneous special pages
'nbytes'            => '$1 {{PLURAL:$1|ба́итъ|ба́ита|ба́итъ}}',
'nlinks'            => '$1 {{PLURAL:$1|съвѧ́ꙁь|съвѧ́ꙁѧ|съвѧ́ꙁи}}',
'listusers'         => 'по́льꙃєватєлъ катало́гъ',
'newpages'          => 'но́ви члѣ́ни',
'newpages-username' => 'по́льꙃєватєлꙗ и́мѧ :',
'move'              => 'прѣимєнова́ниѥ',
'movethispage'      => 'си страни́цѧ прѣимєнова́ниѥ',

# Book sources
'booksources-go' => 'прѣиди́',

# Special:Log
'specialloguserlabel'  => 'по́льꙃєватєл҄ь:',
'speciallogtitlelabel' => 'страни́цѧ и́мѧ :',
'log'                  => 'їсторі́ѩ',
'all-logs-page'        => 'вьсѩ́ їсторі́ѩ',
'log-search-submit'    => 'прѣиди́',

# Special:AllPages
'allpages'       => 'вьсѩ́ страни́цѧ',
'alphaindexline' => 'о́тъ $1 до $2',
'allpagesfrom'   => 'страни́цѧ видѣ́ти хощѫ́ съ начѧ́льнами боу́къвами :',
'allarticles'    => 'вьсѩ́ страни́цѧ',
'allpagessubmit' => 'прѣиди́',

# Special:Categories
'categories' => 'катигорі́ѩ',

# E-mail user
'emailuser' => 'Посъли епїстолѫ',

# Watchlist
'watchlist'      => 'моꙗ́ блюдє́ниꙗ',
'mywatchlist'    => 'Моꙗ́ блюдє́ниꙗ',
'watchlistfor'   => "(по́льꙃєватєлꙗ и́мѧ '''$1''' ѥ́стъ)",
'addedwatchtext' => "страни́ца ⁖ [[:$1]] ⁖ нꙑнѣ по́дъ твоимь [[Special:Watchlist|блюдє́ниѥмь]] ѥ́стъ ⁙
всꙗ ѥѩ и ѥѩжє бєсѣдꙑ мѣ́нꙑ страни́цѧ ⁖ [[Special:Watchlist|моꙗ́ блюдє́ниꙗ]] ⁖ покаꙁанꙑ сѫ́тъ и  [[Special:RecentChanges|послѣ́дьнъ мѣ́нъ]] ката́лоꙃѣ '''чрьнꙑимъ''' сѧ авлꙗѭтъ",
'watch'          => 'блюдє́ниѥ',
'watchthispage'  => 'си страни́цѧ блюдє́ниѥ',
'unwatch'        => 'оста́ви блюдє́ниѥ',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'блюдє́ниѥ ...',
'unwatching' => 'оставьлє́ниѥ блюдє́ниꙗ ...',

'created' => 'сътворѥ́нъ ѥ́стъ',

# Delete/protect/revert
'deletepage'          => 'поничьжє́ниѥ',
'excontent'           => "вънѫтри бѣ: '$1'",
'excontentauthor'     => "вънѫтри́ бѣ : '$1' (и послѣ́дьн҄ии дѣ́тєл҄ь бѣ '[[Special:Contributions/$2|$2]]')",
'delete-legend'       => 'поничьжє́ниѥ',
'actioncomplete'      => 'дѣ́иство сътворєно́ ѥ́стъ',
'deletedtext'         => 'страни́ца ⁖ <nowiki>$1</nowiki> ⁖ поничьжєна ѥ́стъ ⁙ 
виждь ⁖ $2 ⁖ послѣ́дьнъ поничьжє́ниѩ дѣлꙗ́',
'deletedarticle'      => '⁖ [[$1]] ⁖ поничьжє́нъ ѥ́стъ',
'dellogpage'          => 'поничьжє́ниꙗ їсторі́ꙗ',
'deletionlog'         => 'поничьжє́ниꙗ їсторі́ꙗ',
'protectlogpage'      => 'ꙁабранѥ́ниꙗ їсторі́ꙗ',
'protect-level-sysop' => 'то́лико съмотри́тєлє',

# Restrictions (nouns)
'restriction-edit'   => 'испра́ви',
'restriction-move'   => 'прѣимєнова́ниѥ',
'restriction-upload' => 'положє́ниѥ',

# Undelete
'undelete-search-submit' => 'ищи́',

# Namespace form on various pages
'blanknamespace' => '(гла́вьно)',

# Contributions
'contributions' => 'по́льꙃєватєлꙗ добродѣꙗ́ниꙗ',
'mycontris'     => 'моꙗ́ добродѣꙗ́ниꙗ',
'contribsub2'   => 'по́льꙃєватєлꙗ и́мѧ ⁖ $1 ⁖ ѥ́стъ ($2)',
'uctop'         => '(послѣ́дьнꙗ мѣ́на)',

'sp-contributions-blocklog' => 'ꙁаграждє́ниꙗ їсторі́ꙗ',
'sp-contributions-submit'   => 'ищи́',

# What links here
'whatlinkshere'           => 'дос̑ьдє́щьнѩ съвѧ́ꙁи',
'whatlinkshere-title'     => 'страни́цѧ ижє съ ⁖ $1 ⁖ съвѧ́ꙁи имѫтъ',
'whatlinkshere-page'      => 'страни́ца :',
'isredirect'              => 'прѣнаправлѥ́ниѥ',
'whatlinkshere-links'     => '← съвѧ́ꙁи',
'whatlinkshere-hidelinks' => '$1 съвѧ́ꙁи',

# Block/unblock
'blockip'            => 'ꙁагради́ по́льꙃєватєл҄ь',
'ipblocklist-submit' => 'иска́ниѥ',
'blocklink'          => 'ꙁагради́',
'contribslink'       => 'добродѣꙗ́ниꙗ',
'blocklogpage'       => 'ꙁаграждє́ниꙗ їсторі́ꙗ',

# Move page
'move-page'        => 'прѣимєнова́ниѥ ⁖ $1 ⁖',
'move-page-legend' => 'страни́цѧ прѣимєнова́ниѥ',
'movearticle'      => 'страни́ца :',
'newtitle'         => 'но́во и́мѧ :',
'move-watch'       => 'си страни́цѧ блюдє́ниѥ',
'movepagebtn'      => 'прѣимєнова́ниѥ',
'pagemovedsub'     => 'прѣимєнова́ниѥ сътворѥно́ ѥ́стъ',
'movepage-moved'   => "<big>'''⁖ $1 ⁖ нарєчє́нъ ⁖ $2⁖ ѥ́стъ'''</big>", # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'movetalk'         => 'си страни́цѧ бєсѣ́дꙑ прѣимєнова́ниѥ',
'1movedto2'        => '⁖ [[$1]] ⁖ нарєчє́нъ ⁖ [[$2]] ⁖ ѥ́стъ',
'1movedto2_redir'  => '[[$1]] нарєчє́нъ [[$2]] врьхоу́ прѣнаправлѥ́ниꙗ ѥ́стъ.',
'movelogpage'      => 'прѣимєнова́ниꙗ їсторі́ꙗ',

# Namespace 8 related
'allmessages'     => 'сѷсти́мьнꙑ напьса́ниꙗ',
'allmessagesname' => 'и́мѧ',

# Tooltip help for the actions
'tooltip-pt-mytalk'       => 'моꙗ́ бєсѣ́дꙑ страни́ца',
'tooltip-pt-logout'       => 'ис̾хо́дъ',
'tooltip-ca-viewsource'   => 'си страни́ца ꙁабранєна́ ѥ́стъ ⁙
ѥѩ исто́чьнъ о́браꙁъ ви́дєти мо́жєщи',
'tooltip-ca-protect'      => 'си страни́цѧ ꙁабранє́ниѥ',
'tooltip-ca-delete'       => 'си страни́цѧ поничьжє́ниѥ',
'tooltip-ca-move'         => 'си страни́цѧ прѣимєнова́ниѥ',
'tooltip-ca-watch'        => 'си страни́цѧ блюдє́ниѥ',
'tooltip-p-logo'          => 'гла́вьна страни́ца',
'tooltip-n-recentchanges' => 'послѣ́дьнъ мѣ́нъ катало́гъ',
'tooltip-t-upload'        => 'положє́ниѥ дѣ́лъ',
'tooltip-watch'           => 'си страни́цѧ блюдє́ниѥ',

# Media information
'file-info-size' => '($1 × $2 п҃ѯ · дѣ́ла мѣ́ра : $3 · MIME тѵ́пъ : $4)',
'svg-long-desc'  => '(дѣ́ло SVG · обꙑ́чьнъ о́браꙁъ : $1 × $2 п҃ѯ · дѣ́ла мѣ́ра : $3)',
'show-big-image' => 'пль́нъ ви́да о́браꙁъ',

# Special:NewImages
'ilsubmit' => 'ищи́',

# EXIF tags
'exif-artist' => 'творь́ць',

# 'all' in various places, this might be different for inflected languages
'watchlistall2' => 'вьсꙗ́',
'namespacesall' => 'вьсꙗ́',
'monthsall'     => 'вьсѩ́',

'unit-pixel' => 'п҃ѯ',

# Multipage image navigation
'imgmultigo' => 'прѣиди́',

# Table pager
'table_pager_limit_submit' => 'прѣиди́',

# Auto-summaries
'autosumm-new' => 'но́ва страни́ца : $1',

# Size units
'size-bytes' => '$1 Б҃',

# Special:Version
'version'                  => 'MediaWiki о́браꙁъ', # Not used as normal message but as header for the special page itself
'version-version'          => 'о́браꙁъ',
'version-license'          => 'прощє́ниѥ',
'version-software-version' => 'о́браꙁъ',

# Special:FilePath
'filepath-page' => 'дѣ́ло :',

# Special:FileDuplicateSearch
'fileduplicatesearch-submit' => 'ищи́',

# Special:SpecialPages
'specialpages' => 'наро́чьнꙑ страни́цѧ',

);
