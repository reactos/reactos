<?php
/** Kashubian (Kaszëbsczi)
 *
 * @ingroup Language
 * @file
 *
 * @author MinuteElectron
 * @author Warszk
 * @author לערי ריינהארט
 */

$namespaceNames = array(
	NS_MEDIA            => 'Media',
	NS_SPECIAL          => 'Specjalnô',
	NS_MAIN             => '',
	NS_TALK             => 'Diskùsëjô',
	NS_USER             => 'Brëkòwnik',
	NS_USER_TALK        => 'Diskùsëjô_brëkòwnika',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => 'Diskùsëjô_$1',
	NS_IMAGE            => 'Òbrôzk',
	NS_IMAGE_TALK       => 'Diskùsëjô_òbrôzków',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'Diskùsëjô_MediaWiki',
	NS_TEMPLATE         => 'Szablóna',
	NS_TEMPLATE_TALK    => 'Diskùsëjô_Szablónë',
	NS_HELP             => 'Pòmòc',
	NS_HELP_TALK        => 'Diskùsëjô_Pòmòcë',
	NS_CATEGORY         => 'Kategòrëjô',
	NS_CATEGORY_TALK    => 'Diskùsëjô_Kategòrëji'
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Pòdsztrëchiwùjë lënczi:',
'tog-highlightbroken'         => 'Fòrmatëje pùsti lënczi <a href="" class="new">jak nen</a> (alternatiwno: jak nen<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Wërównanié (justifikacëjô) paragrafów',
'tog-hideminor'               => 'Zatacë môłi edicëje w slédnëch zmianach',
'tog-extendwatchlist'         => 'Rozszérzë lëstã ùzérónëch artiklów bë pòkazac wszëtczé zmianë',
'tog-usenewrc'                => 'Rozszérzenié slédnëch zmianów (JavaScript)',
'tog-numberheadings'          => 'Aùtomatné numerowanié nôgłówków',
'tog-showtoolbar'             => 'Pòkażë lëstwã nôrzãdzów edicëji (JavaScript)',
'tog-editondblclick'          => 'Editëjë starnë bez dëbeltné klëkniãcé (JavaScript)',
'tog-editsection'             => 'Włącziwô edicëjã sekcëjów bez lënczi [edicëjô]',
'tog-editsectiononrightclick' => 'Włączë edicëjã sekcëji bez klëkniãcé prawą knąpą mëszë<br />na titlu sekcëji (JavaScript)',
'tog-showtoc'                 => 'Pòkażë spisënk zamkłoscë (dlô starnów z wicy jak 3 nôgłówkama)',
'tog-rememberpassword'        => 'Wdôrzë mòjé miono brëkòwnika na tim kòmpùtrze',
'tog-editwidth'               => 'Kastka edicëji mô fùl szérz',
'tog-watchcreations'          => 'Dodôwôj starnë jaczé ùsôdzã do mòji lëstë ùzérónëch artiklów',
'tog-watchdefault'            => 'Dodôwôj starnë jaczé editëjã do mòji lëstë ùzérónëch artiklów',
'tog-watchmoves'              => 'Dodôwôj starnë jaczé przenoszã do mòji lëstë ùzérónëch artiklów',
'tog-watchdeletion'           => 'Dodôwôj starnë jaczé rëmóm do mòji lëstë ùzérónëch artiklów',
'tog-minordefault'            => 'Zaznaczë wszëtczé edicëje domëslno jakno môłé',
'tog-previewontop'            => 'Pòkażë pòdzérk przed kastką edicëji',
'tog-previewonfirst'          => 'Pòkażë pòdzérk ju przed pierszą edicëją',
'tog-nocache'                 => 'Wëłączë trzëmanié starnów w pamiãcë (caching)',
'tog-enotifwatchlistpages'    => 'Wëslë mie e-mail czedë starna jaką ùzéróm je zmieniwónô',
'tog-enotifusertalkpages'     => 'Wëslë mie e-mail czedë zmieniwónô je mòja starna diskùsëji',
'tog-enotifminoredits'        => 'Wëslë mie e-mail téż dlô môłich zmianów starnów',
'tog-enotifrevealaddr'        => 'Pòkażë mòją adresã e-mail w òdkôzëwùjącym mailu',
'tog-shownumberswatching'     => 'Pòkażë lëczba ùzérającëch brëkòwników',
'tog-fancysig'                => 'Prosti pòdpisënk (bez aùtomatnëch lënków)',
'tog-externaleditor'          => 'Brëkùjë domëslno bùtnowégò editora',
'tog-externaldiff'            => 'Brëkùjë domëslno bùtnowégò nôrzãdza diff',
'tog-showjumplinks'           => 'Włączë lënczi przëstãpù "òbaczë téż"',
'tog-uselivepreview'          => 'Brëkùjë wtimczasnegò pòdzérkù (JavaScript) (eksperimentalné)',
'tog-forceeditsummary'        => 'Pëtôj przed wéńdzenim do pùstégò pòdrechòwania edicëji',
'tog-watchlisthideown'        => 'Zatacë mòjé edicëje z lëstë ùzérónëch artiklów',
'tog-watchlisthidebots'       => 'Zatacë edicëje botów z lëstë ùzérónëch artiklów',
'tog-watchlisthideminor'      => 'Zatacë môłi zmianë z lëstë ùzérónëch artiklów',
'tog-nolangconversion'        => 'Wëłączë kònwersëjã wariantów',
'tog-ccmeonemails'            => 'Wëslë mie kòpije e-mailów jaczi sélóm do jinëch brëkòwników',
'tog-diffonly'                => 'Nie pòkazëjë zamkłoscë starnë niżi różnic',

'underline-always'  => 'Wiedno',
'underline-never'   => 'Nigdë',
'underline-default' => 'Domëslny przezérnik',

'skinpreview' => '(Pòdzérk)',

# Dates
'sunday'        => 'niedzéla',
'monday'        => 'pòniédzôłk',
'tuesday'       => 'wtórk',
'wednesday'     => 'strzoda',
'thursday'      => 'czwiôrtk',
'friday'        => 'piątk',
'saturday'      => 'sobòta',
'sun'           => 'nie',
'mon'           => 'pòn',
'tue'           => 'wtó',
'wed'           => 'str',
'thu'           => 'czw',
'fri'           => 'pią',
'sat'           => 'sob',
'january'       => 'stëcznik',
'february'      => 'gromicznik',
'march'         => 'strëmiannik',
'april'         => 'łżëkwiôt',
'may_long'      => 'môj',
'june'          => 'czerwińc',
'july'          => 'lëpinc',
'august'        => 'zélnik',
'september'     => 'séwnik',
'october'       => 'rujan',
'november'      => 'lëstopadnik',
'december'      => 'gòdnik',
'january-gen'   => 'stëcznika',
'february-gen'  => 'gromicznika',
'march-gen'     => 'strumiannika',
'april-gen'     => 'łżëkwiôta',
'may-gen'       => 'maja',
'june-gen'      => 'czerwińca',
'july-gen'      => 'lëpinca',
'august-gen'    => 'zélnika',
'september-gen' => 'séwnika',
'october-gen'   => 'rujana',
'november-gen'  => 'lëstopadnika',
'december-gen'  => 'gòdnika',
'jan'           => 'stë',
'feb'           => 'gro',
'mar'           => 'str',
'apr'           => 'łżë',
'may'           => 'maj',
'jun'           => 'cze',
'jul'           => 'lëp',
'aug'           => 'zél',
'sep'           => 'séw',
'oct'           => 'ruj',
'nov'           => 'lës',
'dec'           => 'gòd',

# Categories related messages
'pagecategories'        => '{{PLURAL:$1|Kategòrëjô|Kategòrëje}}',
'category_header'       => 'Artikle w kategòrëji "$1"',
'subcategories'         => 'Pòdkategòrëje',
'category-media-header' => 'Media w kategòrëji "$1"',
'category-empty'        => "''Ta ktegòrëja nie zamëkô w se terô niżódnëch artiklów ni mediów.''",

'mainpagetext' => "<big>'''MediaWiki òsta zainstalowónô.'''</big>",

'about'          => 'Ò serwise',
'article'        => 'Artikel',
'newwindow'      => '(òtmëkô sã w nowim òczenkù)',
'cancel'         => 'Anulujë',
'qbfind'         => 'Nalézë',
'qbbrowse'       => 'Przezeranié',
'qbedit'         => 'Edicëjô',
'qbpageoptions'  => 'Òptacëje starnë',
'qbpageinfo'     => 'Ò starnie',
'qbmyoptions'    => 'Mòje òptacëje',
'qbspecialpages' => 'Specjalné starnë',
'moredotdotdot'  => 'Wicy...',
'mypage'         => 'Mòja starna',
'mytalk'         => 'Diskùsëjô',
'anontalk'       => 'Diskùsëjô dlô ti IP-adresë',
'navigation'     => 'Nawigacëjô',
'and'            => 'ë',

# Metadata in edit box
'metadata_help' => 'Metadata:',

'errorpagetitle'   => 'Brida',
'returnto'         => 'Wôrcë sã do starnë: $1.',
'tagline'          => 'Z {{SITENAME}}',
'help'             => 'Pòmòc',
'search'           => 'Szëkba',
'searchbutton'     => 'Szëkba',
'go'               => 'Biôj!',
'searcharticle'    => 'Biôj!',
'history'          => 'Historëjô starnë',
'history_short'    => 'Historëjô',
'updatedmarker'    => 'aktualizowóne òd mòji slédny wizytë',
'info_short'       => 'Wëdowiédza',
'printableversion' => 'Wersëjô do drëkù',
'print'            => 'Drëkùjë',
'edit'             => 'Edicëjô',
'editthispage'     => 'Editëjë ną starnã',
'delete'           => 'Rëmôj',
'deletethispage'   => 'Rëmôj tã starnã',
'protect'          => 'Zazychrëjë',
'unprotect'        => 'Òdzychrëjë',
'talkpagelinktext' => 'Diskùsëjô',
'specialpage'      => 'Specjalnô starna',
'personaltools'    => 'Priwatné przërëchtënczi',
'postcomment'      => 'Dôj dopòwiesc',
'articlepage'      => 'Starna artikla',
'talk'             => 'Diskùsëjô',
'views'            => 'Pòdzérków',
'toolbox'          => 'Przërëchtënczi',
'imagepage'        => 'Starna òbrôzka',
'viewtalkpage'     => 'Starna diskùsëji',
'otherlanguages'   => 'W jinëch jãzëkach',
'redirectedfrom'   => '(Przeczerowóné z $1)',
'lastmodifiedat'   => 'Na starna bëła slédno editowónô ò $2, $1;', # $1 date, $2 time
'viewcount'        => 'Na starna je òbzéranô ju {{PLURAL:$1|jeden rôz|$1 razy}}',
'protectedpage'    => 'Starna je zazychrowónô',
'jumpto'           => 'Skòczë do:',
'jumptonavigation' => 'nawigacëji',
'jumptosearch'     => 'szëkbë',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Ò {{SITENAME}}',
'aboutpage'            => 'Project:Ò_{{SITENAME}}',
'copyright'            => 'Zamkłosc hewòtny starnë je ùżëczónô wedle reglów $1.',
'disclaimers'          => 'Prawné zastrzedżi',
'disclaimerpage'       => 'Project:General_disclaimer',
'edithelp'             => 'Pòmòc do edicëji',
'mainpage'             => 'Przédnô starna',
'mainpage-description' => 'Przédnô starna',
'portal'               => 'Pòrtal wëcmaniznë',
'portal-url'           => 'Project:Pòrtal wëcmaniznë',

'badaccess' => 'Procëmprawne ùdowierzenie',

'versionrequired'     => 'Wëmôgónô wersëjô $1 MediaWiki',
'versionrequiredtext' => 'Bë brëkòwac ną starnã wëmôgónô je wersëjô $1 MediaWiki. Òbaczë starnã [[Special:Version]]',

'ok'                      => 'Jo!',
'youhavenewmessages'      => 'Môsz $1 ($2).',
'newmessageslink'         => 'nowe wiadła',
'youhavenewmessagesmulti' => 'Môsz nowé klëczi: $1',
'editsection'             => 'Edicëjô',
'editold'                 => 'Edicëjô',
'toc'                     => 'Spisënk zamkłoscë',
'showtoc'                 => 'pokôż',
'hidetoc'                 => 'zatacë',
'viewdeleted'             => 'Òbaczë $1',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Artikel',
'nstab-user'      => 'Starna brëkòwnika',
'nstab-special'   => 'Specjalnô',
'nstab-project'   => 'meta-starna',
'nstab-image'     => 'Òbrôzk',
'nstab-mediawiki' => 'ògłosënk',
'nstab-template'  => 'Szablóna',
'nstab-help'      => 'Pòmòc',
'nstab-category'  => 'Kategòrëjô',

# Main script and global functions
'nosuchactiontext'  => 'Programa Mediawiki nie rozpòznôwô taczi òperacëji jakô je w URL',
'nosuchspecialpage' => 'Nie da taczi specjalny starnë',

# General errors
'error'            => 'Fela',
'databaseerror'    => 'Fela w pòdôwkòwi baze',
'readonly'         => 'Baza pòdôwków je zablokòwónô',
'internalerror'    => 'Bënowô fela',
'filecopyerror'    => 'Ni mòże skòpérowac lopka "$1" do "$2".',
'filerenameerror'  => 'Ni mòże zmienic miona lopka "$1" na "$2".',
'filedeleteerror'  => 'Ni mòże rëmac lopka "$1".',
'filenotfound'     => 'Ni mòże nalezc lopka "$1".',
'formerror'        => 'Fela: ni mòże wëslac fòrmùlara',
'badarticleerror'  => 'Nie dô zrobic ti akcëji na ti starnie.',
'badtitle'         => 'Òchëbny titel',
'badtitletext'     => 'Pòdóny titel starnë je òchëbny. Gwësno są w nim znaczi, chtërnëch brëkòwanié je zakôzané abò je pùsti.',
'viewsource'       => 'Zdrojowi tekst',
'viewsourcefor'    => 'dlô $1',
'editinginterface' => "'''ÒSTRZÉGA:''' Editëjesz starnã jaka je brëkòwónô dlô dotegòwóniô tekstu interfejsu dlô soft-wôrë. Wszëtczé zmianë tu zrobioné bãdze widzec na interfejse brëkòwnika.",

# Login and logout pages
'logouttitle'                => 'Wëlogòwanié brëkòwnika',
'logouttext'                 => 'Të jes ju wëlogòwóny. Mòżesz prôcowac z {{SITENAME}} jakno anonimòwi brëkòwnik abò wlogòwac sã jakno zaregistrowóny brëkòwnik.',
'loginpagetitle'             => 'Logòwanié brëkòwnika',
'yourname'                   => 'Miono brëkòwnika',
'yourpassword'               => 'Twòja parola',
'yourpasswordagain'          => 'Pòwtórzë parolã',
'yourdomainname'             => 'Twòjô domena',
'login'                      => 'Wlogùjë mie',
'nav-login-createaccount'    => 'Logòwanié',
'loginprompt'                => "Brëkùjesz miec ''cookies'' (kùszczi) włączoné bë sã wlogòwac do {{SITENAME}}.",
'userlogin'                  => 'Logòwanié',
'logout'                     => 'Wëlogùjë mie',
'userlogout'                 => 'Wëlogòwanié',
'notloggedin'                => 'Felëje logòwóniô',
'nologin'                    => 'Nié môsz logina? $1.',
'nologinlink'                => 'Ùsôdzë kònto',
'createaccount'              => 'Założë nowé kònto',
'gotaccount'                 => 'Masz ju kònto? $1.',
'gotaccountlink'             => 'Wlogùjë',
'createaccountmail'          => 'òb e-mail',
'badretype'                  => 'Wprowadzone parole jinaczą sã midze sobą.',
'userexists'                 => 'To miono brëkòwnika je ju w ùżëcym. Proszã wëbrac jiné miono.',
'username'                   => 'Miono brëkòwnika:',
'uid'                        => 'ID brëkòwnika:',
'yourrealname'               => 'Twòje jistné miono*',
'yourlanguage'               => 'Twój jãzëk:',
'yourvariant'                => 'Wariant:',
'yournick'                   => 'Przezwëstkò (nick):',
'badsig'                     => 'Òchëbny pòdpisënk, sprôwdzë tadżi HTML.',
'badsiglength'               => 'To miono je za dłudżé. Mô bëc mni jakno $1 céchów.',
'prefs-help-realname'        => 'Prôwdzewi miono je òptacjowé a czej je dôsz, òstanié ùżëté do pòdpisaniô Twòjégò wkłôdu',
'loginerror'                 => 'Fela logòwaniô',
'prefs-help-email'           => 'Adresa e-mail je òptacëjnô, le pòzwôlô òna jinëm na kòntakt z Tobą bez starnã brëkòwnika abò starnã diskùsëji nie pòkazëjąc Twòjich pòdôwków.',
'loginsuccesstitle'          => 'ùdałé logòwanié',
'loginsuccess'               => 'Të jes wlogòwóny do {{SITENAME}} jakno "$1".',
'nosuchuser'                 => 'Nie da taczégò brëkòwnika "$1". Sprôwdzë pisënk abò wëfùlujë fòrmular bë założëc nowé kònto.',
'passwordremindertext'       => 'Chtos (prôwdëjuwerno Të, z adresë $1) pòprosëł ò wësłanié nowi parolë dopùscënkù do {{SITENAME}} ($4). Aktualnô parola dlô brëkòwnika "$2" je "$3". Nôlepi mdze czej wlogùjesz sã terô ë zarô zmienisz parolã.',
'noemail'                    => 'W baze ni ma email-adresë dlô brëkòwnika "$1".',
'acct_creation_throttle_hit' => 'Môsz zrobiony ju $1 kontów. Nie mòżesz miec ju wicy.',
'emailauthenticated'         => 'Twòjô adresa e-mail òsta pòcwierdzonô $1.',
'accountcreated'             => 'Konto założone',
'accountcreatedtext'         => 'Konto brëkòwnika dlô $1 je założone.',

# Edit page toolbar
'bold_sample'   => 'Wëtłëszczony drëk',
'bold_tip'      => 'Wëtłëszczony drëk',
'nowiki_sample' => 'Wstôw tuwò niesfòrmatowóny tekst',
'nowiki_tip'    => 'Ignorëjë wiki-fòrmatowanié',
'hr_tip'        => 'Wòdorównô (horizontalnô) linijô (brëkùjë szpôrowno)',

# Edit pages
'summary'               => 'Pòdrechòwanié',
'minoredit'             => 'Drobnô edicëjô.',
'watchthis'             => 'Ùzérôj',
'savearticle'           => 'Zapiszë artikel',
'preview'               => 'Pòdzérk',
'showpreview'           => 'Pòdzérk',
'showlivepreview'       => 'Pòdzérk',
'showdiff'              => 'Pòkażë zmianë',
'anoneditwarning'       => "'''Bôczë:''' Të nie je wlogòwóny. Twòjô adresa IP mdze zapisónô w historëji edicëji ti starnë.",
'blockedtitle'          => 'Brëkòwnik je zascëgóny',
'blockedtext'           => "Twòje kònto abò ë IP-adresa òstałë zascëgòwóné przez $1. Pòdónô przëczëna to:<br />''$2''.<br />Bë zgwësnic sprawã zablokòwaniô mòżesz skòntaktowac sã z $1 abò jińszim [[{{MediaWiki:Grouppage-sysop}}|administratorã]].

Boczë, że të nie mòżesz stądka sélac e-mailów, jeżlë nié môsz jesz zaregisterowóné e-mailowé adresë w [[Special:Preferences|nastôwach]].

Twòjô adresa IP to $3. Proszã dodôj nã adresã we wszëtczich pëtaniach.",
'blockedoriginalsource' => "Zdrój '''$1''' je niżi:",
'blockededitsource'     => "Tekst '''Twòjëch edicëji''' do '''$1''' je niżi:",
'whitelistedittitle'    => 'Bë editowac je nót sã wlogòwac',
'accmailtitle'          => 'Parola wësłónô.',
'accmailtext'           => 'Parola dlô "$1" je wësłónô do $2.',
'newarticletext'        => "Môsz przëszłi z lënkù do starnë jaka jesz nie òbstoji.
Bë ùsôdzëc artikel, naczni pisac w kastce niżi (òb. [[{{MediaWiki:Helppage}}|starnã pòmòcë]]
dlô wicy wëdowiédzë).
Jeżlë jes të tuwò bez zmiłkã, le klëkni w swòjim przezérnikù knąpã '''nazôd'''.",
'anontalkpagetext'      => "----''To je starna dyskùsëji anonimòwiégò brëkòwnika, chtëren nie zrobił jesz kònta dlô se, abò gò nie brëkùje.
Takô adresa IP, mòże bëc brëkòwónô òb wiele lëdzy.
Eżlë klëczi na ti starnie nie są sczérowóne do ce, tedë [[Special:UserLogin|zrobi sobie nowé kònto]] abò zalogùje sã, bë niechac zmiłczi z jinëma anonimòwima brëkòwnikama.''",
'clearyourcache'        => "'''Bôczë:''' Pò zapisanim, mòże bãdzesz mùszôł òminąc pamiãc przezérnika bë òbaczëc zmianë. '''Mozilla / Firefox / Safari:''' przëtrzëmôj ''Shift'' òbczas klëkaniô na ''Reload'', abò wcësni ''Ctrl-Shift-R'' (''Cmd-Shift-R'' na kòmpùtrach Mac); '''IE:''' przëtrzëmôj ''Ctrl'' òbczas klëkaniô na ''Refresh'', abò wcësni ''Ctrl-F5''; '''Konqueror''': prosto klëkni na knąpã ''Reload'', abò wcësni ''F5''; brëkòwnicë '''Operë''' bãdą mést mùszële wëczëszczëc pamiãc w ''Tools→Preferences''.",
'previewnote'           => '<strong>To je blós pòdzérk - artikel jesz nie je zapisóny!</strong>',
'editing'               => 'Edicëjô $1',
'explainconflict'       => 'Chtos sfórtowôł wprowadzëc swòją wersëjã artikla òbczôs Twòji edicëji. Górné pòle edicëji zamëkô w se tekst starnë aktualno zapisóny w pòdôwkòwi baze. Twòje zmianë są w dólnym pòlu edicëji. Bë wprowadzëc swòje zmianë mùszisz zmòdifikòwac tekst z górnégò pòla. <b>Blós</b> tekst z górnégò pòla mdze zapisóny w baze czej wcësniesz "Zapiszë".',
'yourtext'              => 'Twój tekst',
'yourdiff'              => 'Zjinaczi',
'copyrightwarning'      => 'Bôczë, że wszëtczé edicëje w {{SITENAME}} są wprowadzané pòd zastrzégą $2 (òb. $1 dlô detalów). Jeżlë nie chcesz bë to co napiszesz bëło editowóné czë kòpijowóné, tedë nie zacwierdzôj nëch edicëjów.<br />Zacwierdzając zmianë dôwôsz parolã, że to co môsz napisóné je Twòjégò aùtorstwa, abò skòpijowóné z dostónków public domain abò jinëch wòlnëch licencëjów. <strong>NIE DODÔWÔJ CËZËCH TEKSTÓW BEZ ZEZWÒLENIÔ!</strong>',
'copyrightwarning2'     => 'Bôczë, że wszëtczé edicëje w {{SITENAME}} mògą bëc editowóné, zmienióné abò rëmniãté bez jinëch brëkòwników. Jeżlë nie chcesz bë Twòja robòta bëła editowónô, tedë nie zacwierdzôj nëch edicëjów.<br />Zacwierdzając zmianë dôwôsz parolã, że to co môsz napisóné je Twòjégò aùtorstwa, abò skòpijowóné z dostónków public domain abò jinëch wòlnëch licencëjów. <strong>NIE DODÔWÔJ CËZËCH TEKSTÓW BEZ ZEZWÒLENIÔ!</strong>',
'readonlywarning'       => 'BÔCZËNK: Pòdôwkòwô baza òsta sztërkòwô zablokòwónô dlô administracëjnëch célów. Nie mòże tej timczasã zapisac nowi wersëje artikla. Bédëjemë przeniesc ji tekst do priwatnégò lopka
(wëtnij/wstôw) ë zachòwac na pózni.',
'templatesused'         => 'Szablónë ùżëti w tim artiklu:',

# History pages
'cur'        => 'aktualnô',
'last'       => 'pòslédnô',
'histlegend' => 'Legenda: (aktualnô) = różnice w przërównanim do aktualny wersëje,
(wczasniészô) = różnice w przërównanim do wczasniészi wersëje, D = drobné edicëje',

# Diffs
'difference'              => '(różnice midzë wersëjama)',
'lineno'                  => 'Lëniô $1:',
'compareselectedversions' => 'Przërównôj wëbróné wersëje',

# Search results
'noexactmatch' => "'''Nie dô starnë z dokładno taczim titlã \"\$1\"'''. Mòżesz [[:\$1|zrobic ną starnã]].",
'viewprevnext' => 'Òbaczë ($1) ($2) ($3).',
'powersearch'  => 'Szëkba',

# Preferences page
'preferences'           => 'Preferencëje',
'mypreferences'         => 'Mòje nastôwë',
'prefs-edits'           => 'Lëczba edicëjów:',
'prefsnologin'          => 'Felënk logòwóniô',
'qbsettings'            => 'Sztrépk chùtczégò przistãpù',
'changepassword'        => 'Zmiana parolë',
'skin'                  => 'Wëzdrzatk',
'math'                  => 'Matematika',
'dateformat'            => 'Fòrmat datumù',
'datedefault'           => 'Felëje preferencëji',
'datetime'              => 'Datum ë czas',
'math_failure'          => 'Parser nie rozmiôł rozpòznac',
'prefs-personal'        => 'Pòdôwczi brëkòwnika',
'prefs-rc'              => 'Slédné edicëje',
'prefs-watchlist'       => 'Lësta ùzérónëch artiklów',
'prefs-watchlist-days'  => 'Maksymalnô lëczba dniów dlô wëskrzëniwóniô na lësce ùzérónëch artiklów:',
'prefs-watchlist-edits' => 'Maksymalnô lëczba edicëjów do pòkazaniô w rozszérzoné lësce ùzérónëch artiklów:',
'prefs-misc'            => 'Jine',
'saveprefs'             => 'Zapiszë',
'resetprefs'            => 'Wëczëszczë',
'oldpassword'           => 'Stôrô parola:',
'newpassword'           => 'Nowô parola',
'retypenew'             => 'Napiszë nową parolã jesz rôz',
'textboxsize'           => 'Edicëjô',
'rows'                  => 'Régów:',
'columns'               => 'Kòlumnów:',
'searchresultshead'     => 'Szëkba',
'resultsperpage'        => 'Rezultatów na starnã:',
'contextlines'          => 'Régów na rezultat:',
'contextchars'          => 'Kòntekstów na régã:',
'stub-threshold'        => 'Greńca dlô fòrmatowaniô <a href="#" class="stub">lënków stubów</a>:',
'recentchangesdays'     => 'Kùli dni pòkazëwac w slédnëch edicëjach:',
'recentchangescount'    => 'Wielëna pòzycëji na lësce slédnëch edicëji',
'savedprefs'            => 'Twòjé nastôwë òstałë zapisóné.',
'timezonelegend'        => 'Czasowô cona',
'timezonetext'          => '¹Lëczba gòdzënów różnicë midze twòjim môlowim czasã a czasã na serwerze (UTC).',
'localtime'             => 'Twòja czasowô cona',
'timezoneoffset'        => 'Różnica¹',
'servertime'            => 'Aktualny czas serwera',
'guesstimezone'         => 'Wezmi z przezérnika',
'allowemail'            => 'Włączë mòżlewòtã sélaniô e-mailów òd jinëch brëkòwników',
'defaultns'             => 'Domëslno przeszëkùjë nôslédné rëmnotë mionów:',
'files'                 => 'Lopczi',

# User rights
'editinguser' => "Edicëjô brëkòwnika '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",

'grouppage-sysop' => '{{ns:project}}:Administratorzë',

# Recent changes
'recentchanges'                  => 'Slédné edicëje',
'recentchangestext'              => 'Na starna prezentérëje historëjã slédnëch edicëjów w {{SITENAME}}.',
'recentchanges-feed-description' => 'Pòdstrzegô slédny zmianë w tim pòwrózkù.',
'rcnotefrom'                     => "Niżi są zmianë òd '''$2''' (pòkazóné do '''$1''').",
'rclistfrom'                     => 'Pòkażë nowé zmianë òd $1',
'rcshowhideminor'                => '$1 môłé zmianë',
'rcshowhidebots'                 => '$1 botë',
'rcshowhideliu'                  => '$1 zalogòwónëch brëkòwników',
'rcshowhideanons'                => '$1 anonymòwëch brëkòwników',
'rcshowhidepatr'                 => '$1 òbzérónë edicëje',
'rcshowhidemine'                 => '$1 mòjé edicëje',
'rclinks'                        => 'Pòkażë slédnëch $1 zmianów zrobionëch òb slédné $2 dniów<br />$3',
'hide'                           => 'zatacë',
'show'                           => 'pokôż',
'minoreditletter'                => 'D',

# Recent changes linked
'recentchangeslinked' => 'Zmianë w dolënkòwónëch',

# Upload
'upload'            => 'Wladënk lopka',
'reupload'          => 'Wëslë jesz rôz',
'uploadnologin'     => 'Felënk logòwaniô',
'uploadtext'        => '<strong>STOP!</strong> Nigle wladëjesz jaczi lopk,\\nprzeczëtôj regle wladowaniô lopków ë ùgwësnij sã, że wladëwającë gò òstóniesz z\\nnima w zgòdze.\\n<p>Jeżle chcesz przezdrzec abò przeszëkac do terô wladowóné lopczi,\\nprzeńdzë do [[Special:ImageList|lëstë wladowónëch lopków]].\\nWszëtczé wladënczi ë rëmania są òdnotérowóné w\\nspecjalnëch zestôwkach: [[Special:Log/upload|wladënczi]] ë [[Special:Log/delete|rëmóné]].\\n<p>Bë wëslac nowi lopk do zòbrazowaniô Twòjégò artikla wëzwëskùj \\nhewòtny fòrmùlar.\\nW wikszoscë przezérników ùzdrzesz knąpã <i>Browse...</i>\\nabò <i>Przezérôj...</i>, chtëren ùmożlëwi Cë òtemkniãcé sztandardowégò\\nòkna wëbiérkù lopka. Wëbranié lopka sprawi wstôwienié jegò miona\\nw tekstowim pòlu kòl knąpë.\\nZaznaczającë pasowné pòle, mùszisz téż pòcwierdzëc, ëż sélającë\\nlopk nie gwôłcësz nikògò autorsczich praw.\\nWladënk zacznie sã pò wcësniãcym <i>Wladëjë lopk</i>.\\nTo mòże sztërk zdérowac, òsoblëwò jeżle ni môsz chùtczégò dopùscënkù do internetu.\\n<p>Preferowónyma fòrmatama są: JPEG dlô òdjimków, PNG dlô céchùnków\\në òbrôzków ze znankama ikònów, ôs OGG dlô zwãków. Bë nie dac przińc do lëchòrozmieniów nadôwôj lopkom miona sparłãczóné z jich zamkłoscą.\\nBë wstôwic òbrôzk do artikla, wpiszë lënk:\\n<b><nowiki>[[</nowiki>{{ns:image}}<nowiki>:miono.jpg]]</nowiki></b> abò <b><nowiki>[[</nowiki>{{ns:image}}<nowiki>:miono.png|òpcjonalny tekst]]</nowiki></b>.\\nDlô zwãkòwëch lopków lënk mdze wëzdrzôł tak: <b><nowiki>[[</nowiki>{{ns:media}}<nowiki>:file.ogg]]</nowiki></b>.\\n<p>Prosymë wdarzëc, ëż tak samò jak w przëtrôfkù zwëczajnëch starnów {{SITENAME}},\\njińszi brëkòwnicë mògą editowac abò rëmac wladowóné przez Ce lopczi,\\njeżle mdą dbë, że to mdze lepi służëc całi ùdbie {{SITENAME}}.\\nTwòje prawò do sélaniégò lopków mòże bëc Cë òdebróné, eżle nadùżëjesz systemë.',
'uploadlog'         => 'Lësta wladënków',
'uploadlogpage'     => 'Dołączoné',
'uploadlogpagetext' => 'Hewò je lësta slédno wladowónëch lopków.\\nWszëtczé gòdzënë tikają conë ùniwersalnégò czasë.',
'filename'          => 'Miono lopka',
'filedesc'          => 'Òpisënk',
'fileuploadsummary' => 'Pòdrechòwanié:',
'filesource'        => 'Zdrój:',
'uploadedfiles'     => 'Wladowóné lopczi',
'badfilename'       => 'Miono òbrôzka zmienioné na "$1".',
'successfulupload'  => 'Wladënk darzëł sã',
'uploadwarning'     => 'Òstrzega ò wladënkù',
'savefile'          => 'Zapiszë lôpk',
'uploadedimage'     => 'wladënk: "$1"',
'uploaddisabled'    => 'Przeprôszómë! Mòżlëwòta wladënkù lopków na nen serwer òsta wëłączonô.',

# Special:ImageList
'imagelist'      => 'Lësta òbrôzków',
'imagelist_user' => 'Brëkòwnik',

# Image description page
'filehist-user'  => 'Brëkòwnik',
'imagelinks'     => 'Lënczi do lopka',
'linkstoimage'   => 'Hewò są starnë, jaczé òdwòłëją sã do negò lopka:',
'nolinkstoimage' => 'Niżódnô starna nie òdwòłëje sã do negò lopka.',

# Random page
'randompage' => 'Kawlowô starna',

# Statistics
'sitestats'     => 'Statistika artiklów',
'userstats'     => 'Statistika brëkòwników',
'sitestatstext' => "W pòdôwkòwi baze je w sëmie '''$1''' starn.
Na wielëna zamëkô w se starnë ''Diskùsëji'', starnë ò {{SITENAME}}, starnë ôrtë ''stub'' (ùzémk), starnë przeczerowóniô, ë jińszé, chtërné grãdo je klasyfikòwac jakno artikle.
Bez nëch to prôwdëjuwerno da '''$2''' starn artiklów.

'''$8''' lopków òsta załadowónëch.

Bëło w sëmie '''$3''' òdwiôdënów ë '''$4''' edicëji òd sztótu, czej miôł plac upgrade soft-wôrë. Dôwó to strzédno $5 edicëji na jedną starnã ë $6 òdwiôdënów na jedną edicëjã.

Długòta [http://www.mediawiki.org/wiki/Manual:Job_queue rédżi robòtë] je '''$7'''.",

'disambiguationspage' => 'Template:Starnë_ùjednoznacznieniô',

'doubleredirects' => 'Dëbeltné przeczérowania',

'brokenredirects' => 'Zerwóné przeczerowania',

# Miscellaneous special pages
'nlinks'            => '$1 lënków',
'lonelypages'       => 'Niechóné starnë',
'unusedimages'      => 'Nie wëzwëskóné òbrôzczi',
'popularpages'      => 'Nôwidzalszé starnë',
'wantedpages'       => 'Nônótniészé starnë',
'shortpages'        => 'Nôkrótszé starnë',
'longpages'         => 'Nôdłëgszé starnë',
'listusers'         => 'Lësta brëkòwników',
'newpages'          => 'Nowé starnë',
'newpages-username' => 'Miono brëkòwnika:',
'ancientpages'      => 'Nôstarszé starnë',
'move'              => 'Przeniesë',
'movethispage'      => 'Przeniesë',
'notargettitle'     => 'Nie da taczi starnë',

# Book sources
'booksources' => 'Ksążczi',

# Special:Log
'specialloguserlabel' => 'Brëkòwnik:',
'log'                 => 'Lodżi',
'alllogstext'         => 'Sparłãczone registrë wësłónëch lopków, rëmónëch starn, zazychrowaniô, blokòwaniô ë nadôwaniô ùdowierzeniów. Mòżesz zawãżëc wëszłosc òb wëbranié ôrtu registru, miona brëkòwnika abò miona zajimnej dlô ce starnë.',

# Special:AllPages
'allpages'          => 'Wszëtczé starnë',
'alphaindexline'    => '$1 --> $2',
'allpagesfrom'      => 'Starnë naczënające sã na:',
'allarticles'       => 'Wszëtczé artikle',
'allinnamespace'    => 'Wszëtczé starnë (w rumie $1)',
'allnotinnamespace' => 'Wszëtczé starnë (nie w rumie $1)',
'allpagesprev'      => 'Przódnô',
'allpagesnext'      => 'Pòsobnô',
'allpagessubmit'    => 'Pòkôżë',
'allpagesprefix'    => 'Pòkôżë naczënającë sã òd:',

# Special:Categories
'categories' => 'Kategòrëje',

# E-mail user
'emailuser'       => 'Wëslë e-maila do negò brëkòwnika',
'emailpage'       => 'Sélajë e-mail do brëkòwnika',
'defemailsubject' => 'E-mail òd {{SITENAME}}',
'noemailtitle'    => 'Felënk email-adresë',
'emailfrom'       => 'Òd',
'emailto'         => 'Do',
'emailsubject'    => 'Téma',
'emailmessage'    => 'Wiadło',
'emailsend'       => 'Wëslë',

# Watchlist
'watchlist'            => 'Lësta ùzérónëch artiklów',
'mywatchlist'          => 'Lësta ùzérónëch artiklów',
'watchlistfor'         => "(dlô '''$1''')",
'watchnologin'         => 'Felënk logòwóniô',
'addedwatch'           => 'Dodónô do lëstë ùzérónëch',
'addedwatchtext'       => "Starna \"[[:\$1]]\" òsta dodónô do twòji [[Special:Watchlist|lëstë ùzérónëch artiklów]].
Na ti lësce są registre przińdnëch zjinak ti starne ë na ji starnie dyskùsëji, a samò miono starnë mdze '''wëtłëszczone''' na [[Special:RecentChanges|lësce slédnich edicëji]], bë të mògł to òbaczëc. 

Czej chcesz remôc starnã z lëste ùzéronëch artiklów, klikni ''Òprzestôj ùzérac''.",
'removedwatch'         => 'Rëmóné z lëstë ùzérónëch',
'watch'                => 'Ùzérôj',
'watchthispage'        => 'Ùzérôj ną starnã',
'unwatch'              => 'Òprzestôj ùzerac',
'unwatchthispage'      => 'Òprzestôj ùzerac ną starnã',
'notanarticle'         => 'To nie je artikel',
'watchlist-details'    => 'Ùzéróné môsz {{PLURAL:$1|$1 artikel|$1 artikle (-ów)}}, nie rechùjąc diskùsëjów.',
'wlheader-showupdated' => "* Artiklë jakczé òsta zmienioné òd Twòji slédny wizytë są wëapratnioné '''pògrëbieniém'''",
'watchmethod-list'     => 'szëkba ùzérónëch artiklów westrzód pòslédnëch edicëjów',
'watchlistcontains'    => 'Wielëna artiklów na Twòji lësce ùzérónëch: $1.',
'wlnote'               => "Niżi môsz wëskrzënioné {{PLURAL:$1|slédną zmianã|'''$1''' slédnëch zmianów}} zrobioné òb {{PLURAL:$2|gòdzënã|'''$2''' gòdzënë/gòdzënów}}.",
'wlshowlast'           => 'Pòkażë zmianë z $1 gòdzënów $2 dni $3',
'watchlist-show-bots'  => 'Pòkażë edicëje bòtów',
'watchlist-hide-bots'  => 'Zatacë edicëje bòtów',
'watchlist-show-own'   => 'Pòkażë mòjé edicëje',
'watchlist-hide-own'   => 'Zatacë mòjé edicëje',
'watchlist-show-minor' => 'Pòkażë môłé edicëje',
'watchlist-hide-minor' => 'Zatacë môłé edicëje',

'enotif_reset' => 'Òznaczë wszëtczé artiklë jakno òbëzdrzóné',
'changed'      => 'zmienioné',
'created'      => 'zrobionô',

# Delete/protect/revert
'deletepage'         => 'Rëmôj starnã',
'confirm'            => 'Pòcwierdzë',
'excontent'          => 'Zamkłosc starnë "$1"',
'actioncomplete'     => 'Òperacëjô wëkònónô',
'dellogpage'         => 'Rëmóné',
'deletionlog'        => 'register rëmaniów',
'deletecomment'      => 'Przëczëna rëmaniô',
'rollback'           => 'Copnij edicëjã',
'rollbacklink'       => 'copnij',
'rollbackfailed'     => 'Nie szło copnąc zmianë',
'alreadyrolled'      => 'Nie jidze copnąc slédnej zmianë starnë [[:$1]], chtërnej ùsôdzcą je [[User:$2|$2]] ([[User talk:$2|Diskùsëjô]]).
Chtos jiny ju editowôł starnã abò copnął zmianë.

Ùsôdzcą slédnej zmianë je terô [[User:$3|$3]] ([[User talk:$3|Diskùsëjô]]).',
'protectedarticle'   => 'zazychrowónô [[$1]]',
'unprotectedarticle' => 'òdzychrowóny [[$1]]',
'protect-legend'     => 'Pòcwierdzë zazychrowanié',
'protectcomment'     => 'Przëczëna zazychrowóniô',

# Undelete
'viewdeletedpage' => 'Òbaczë rëmóne starnë',

# Namespace form on various pages
'namespace'      => 'Rum mionów:',
'invert'         => 'Òdwrócë zaznaczenié',
'blanknamespace' => '(Przédnô)',

# Contributions
'contributions' => 'Wkłôd brëkòwników',
'mycontris'     => 'Mòje edicëje',
'contribsub2'   => 'Dlô brëkòwnika $1 ($2)',
'uctop'         => '(slédnô)',
'month'         => 'Òd miesąca (ë wczasni):',
'year'          => 'Òd rokù (ë wczasni):',

'sp-contributions-newbies'  => 'Pòkażë edicëjã blós nowich brëkòwników',
'sp-contributions-search'   => 'Szëkba za edicëjama',
'sp-contributions-username' => 'Adresa IP abò miono brëkòwnika:',
'sp-contributions-submit'   => 'Szëkôj',

# What links here
'whatlinkshere' => 'Lënkùjącé',
'linkshere'     => 'Do ny starnë òdwòłëją sã hewòtné starnë:',
'isredirect'    => 'starna przeczerowaniô',

# Block/unblock
'blockip'            => 'Zascëgôj IP-adresã',
'blockiptext'        => 'Brëkùje formùlarza niżi abë zascëgòwac prawò zapisënkù spòd gwësny adresë IP. To robi sã blós dlôte abë zascëgnąc wandalëznom, a bëc w zgòdze ze [[{{MediaWiki:Policy-url}}|wskôzama]]. Pòdôj przëczënã (np. dając miona starn, na chtërnëch dopùszczono sã wandalëzny).',
'ipbreason'          => 'Przëczëna',
'badipaddress'       => 'IP-adresa nie je richtich pòdónô.',
'blockipsuccesssub'  => 'Zascëgónié dało sã',
'blockipsuccesstext' => 'Brëkòwnik [[Special:Contributions/$1|$1]] òstał zascëgóny.<br />
Biéj do [[Special:IPBlockList|lëstë zascëgónëch adresów IP]] abë òbaczëc zascëdżi.',
'blocklistline'      => '$1, $2 zascëgôł $3 ($4)',
'blocklink'          => 'zascëgôj',
'contribslink'       => 'wkłôd',
'autoblocker'        => 'Zablokòwóno ce aùtomatnie, ga brëkùjesz ti sami adresë IP co brëkòwnik "[[User:$1|$1]]". Przëczënô blokòwóniô $1 to: "\'\'\'$2\'\'\'".',
'proxyblocksuccess'  => 'Fertich.',

# Developer tools
'lockbtn' => 'Zascëgôj bazã pòdôwków',

# Move page
'move-page-legend'        => 'Przeniesë starnã',
'movearticle'             => 'Przeniesë artikel',
'movepagebtn'             => 'Przeniesë starnã',
'pagemovedsub'            => 'Przeniesenié darzëło sã',
'articleexists'           => 'Starna ò taczim mionie ju je abò nie je òno bezzmiłkòwé. Wëbierzë nowé miono.',
'movedto'                 => 'przeniesłô do',
'movetalk'                => 'Przeniesë téż starnã <i>Diskùsëje</i>, jeżle je to mòżlëwé.',
'1movedto2'               => '$1 przeniesłé do $2',
'1movedto2_redir'         => '[[$1]] przeniesłé do [[$2]] nad przeczérowanim',
'delete_and_move'         => 'Rëmôj ë przeniesë',
'delete_and_move_confirm' => 'Jo, rëmôj ną starnã',

# Export
'export' => 'Ekspòrt starnów',

# Namespace 8 related
'allmessages'               => 'Wszëtczé systemòwé ògłosë',
'allmessagesname'           => 'Miono',
'allmessagesdefault'        => 'Domëslny tekst',
'allmessagescurrent'        => 'Terny tekst',
'allmessagestext'           => 'To je zestôwk systemòwëch ògłosów przistãpnëch w rumie mion MediaWiki.
Please visit [http://www.mediawiki.org/wiki/Localisation MediaWiki Localisation] and [http://translatewiki.net Betawiki] if you wish to contribute to the generic MediaWiki localisation.',
'allmessagesnotsupportedDB' => "'''{{ns:special}}:Allmessages''' nie mòże bëc brëkòwónô, temù że '''\$wgUseDatabaseMessages''' je wëłączony.",
'allmessagesfilter'         => 'Filter mion ògłosów:',
'allmessagesmodified'       => 'Pòkażë blós zjinaczone',

# Special:Import
'import' => 'Impòrtëjë starnë',

# Tooltip help for the actions
'tooltip-pt-userpage'    => 'Mòja starna brëkòwnika',
'tooltip-pt-mytalk'      => 'Mòja starna diskùsëji',
'tooltip-pt-preferences' => 'Mòje nastôwë',
'tooltip-pt-watchlist'   => 'Lësta artiklów jaczé òbzérôsz za zmianama',
'tooltip-pt-mycontris'   => 'Lësta mòjich edicëjów',
'tooltip-pt-logout'      => 'Wëlogòwanié',
'tooltip-watch'          => 'Dodôj ną starnã do lëstë ùzérónëch',

# Attribution
'anonymous'        => 'Anonimòwi brëkòwnik/-cë  {{SITENAME}}',
'siteuser'         => 'Brëkòwnik {{SITENAME}} $1',
'lastmodifiedatby' => 'Na starna bëła slédno editowónô $2, $1 przez $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Òpiarté na prôcë $1.',
'others'           => 'jiné',

# Spam protection
'spamprotectiontitle' => 'Anti-spamòwi filter',

# Math options
'mw_math_png'    => 'Wiedno wëskrzëniwôj jakno PNG',
'mw_math_simple' => 'Jeżlë prosti wëskrzëniwôj jakno HTML, w jinëm przëtrôfkù jakno PNG',
'mw_math_html'   => 'HTML czej mòżlewé a w jinëm przëtrôfkù PNG',
'mw_math_source' => 'Òstawi jakno TeX (dlô tekstowich przezérników)',
'mw_math_modern' => 'Zalécóné dlô nowoczasnëch przezérników',
'mw_math_mathml' => 'Wëskrzëniwôj jakno MathML jeżlë mòżlëwé (eksperimentalné)',

# Browsing diffs
'previousdiff' => '← Pòprzédnô różnica',
'nextdiff'     => 'Pòstãpnô różnica →',

# Media information
'imagemaxsize' => 'Limitëjë òbrôzczi na starnie òpisënkù òbrôzków do:',
'thumbsize'    => 'Miara miniaturków:',

# Special:NewImages
'ilsubmit' => 'Szëkôj',
'bydate'   => 'wedle datumù',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'wszëtczé',
'imagelistall'     => 'wszëtczé',
'watchlistall2'    => 'wszëtczé',
'namespacesall'    => 'wszëtczé',
'monthsall'        => 'wszëtczé',

# E-mail address confirmation
'confirmemail_loggedin' => 'Twòjô adresa e-mail òsta pòcwierdzona.',

# AJAX search
'articletitles' => "Artikle naczënającë sã na ''$1''",

# Multipage image navigation
'imgmultigo' => 'Biéj!',

# Auto-summaries
'autoredircomment' => 'Przeczérowanié do [[$1]]',

# Watchlist editing tools
'watchlisttools-view' => 'Òbaczë wôżnészé zmianë',
'watchlisttools-edit' => 'Òbaczë a editëjë lëstã ùzérónëch artiklów',
'watchlisttools-raw'  => 'Editëjë sërą lëstã',

# Special:Version
'version' => 'Wersëjô', # Not used as normal message but as header for the special page itself

# Special:SpecialPages
'specialpages' => 'Specjalné starnë',

);
