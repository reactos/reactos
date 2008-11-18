<?php
/** Lumbaart (Lumbaart)
 *
 * @ingroup Language
 * @file
 *
 * @author Amgine
 * @author Clamengh
 * @author Dakrismeno
 * @author DracoRoboter
 * @author Flavio05
 * @author Kemmótar
 * @author Malafaya
 * @author Remulazz
 * @author SabineCretella
 * @author Snowdog
 * @author Sprüngli
 */

$fallback = 'it';

$messages = array(
# User preference toggles
'tog-hideminor'               => 'Scuunt i redatazziún menu impurtaant in di "cambiameent reçeent"',
'tog-usenewrc'                => '"cambiameent reçeent" migliuraa (JavaScript)',
'tog-showtoolbar'             => 'Fá vidé ai butún da redatazziún (JavaScript)',
'tog-editondblclick'          => 'Redatá i pagin cun al dópi clic (JavaScript)',
'tog-editsection'             => 'Ailitá redatazziún dii sezziún atravèerz al ligam [redatá]',
'tog-editsectiononrightclick' => 'Abilitá redatazziún dai sezziún cun al clic<br />
süi titul dai sezziún (JavaScript)',
'tog-rememberpassword'        => "Regòrdass la mè paròla d'urdin",
'tog-editwidth'               => "La finèstra da redatazziún la gh'á dimensiún graant",
'tog-watchdefault'            => "Gjüntá i pagin redataa in dala lista dii pagin tegnüü d'öcc",
'tog-minordefault'            => 'Marcá sempar tücc i redatazziún cuma "da minuur impurtanza"',
'tog-previewontop'            => "Fá vidé un'anteprima anaanz dala finèstra da redatazziún",
'tog-previewonfirst'          => "Fá vidé l'anteprima ala prima redatazziún",
'tog-fancysig'                => 'Firma semplificava (senza al ligamm utumatich)',
'tog-externaleditor'          => 'Druvá sémpar un prugráma da redatazziún esternu',
'tog-externaldiff'            => 'Druvá sempar un "diff" estèrnu',
'tog-watchlisthideown'        => "Sconda i me mudifich dai pagin che a ten d'ögg",
'tog-watchlisthidebots'       => "Sconda i mudifich di bot da i pagin che a ten d'ögg",

'underline-always' => 'Semper',
'underline-never'  => 'Mai',

# Dates
'sunday'        => 'dumeniga',
'monday'        => 'lündesdí',
'wednesday'     => 'mercurdí',
'thursday'      => 'gjöbia',
'friday'        => 'vendredí',
'saturday'      => 'sábat',
'january'       => 'ginee',
'february'      => 'febraar',
'march'         => 'maarz',
'april'         => 'avriil',
'may_long'      => 'macc',
'june'          => 'gjügn',
'july'          => 'lüi',
'august'        => 'avóst',
'september'     => 'setembər',
'october'       => 'utubər',
'november'      => 'nuvembər',
'december'      => 'dicember',
'january-gen'   => 'Giner',
'february-gen'  => 'Fevrer',
'march-gen'     => 'Marz',
'april-gen'     => 'Avril',
'may-gen'       => 'Mag',
'june-gen'      => 'Giugn',
'july-gen'      => 'Luj',
'august-gen'    => 'Aoust',
'september-gen' => 'Setember',
'october-gen'   => 'Otober',
'november-gen'  => 'November',
'december-gen'  => 'Dizember',
'mar'           => 'mrz',
'apr'           => 'avr',
'may'           => 'mac',
'jun'           => 'gjü',
'jul'           => 'lüi',
'aug'           => 'avo',
'oct'           => 'utu',
'nov'           => 'nuv',

# Categories related messages
'pagecategories'  => '{{PLURAL:$1|Categuria|Categurij}}',
'category_header' => 'Vus in de la categuria "$1"',
'subcategories'   => 'Sót-categurii',

'about'          => 'A pruposit də',
'newwindow'      => "(sa derviss in un'óltra finèstra)",
'cancel'         => 'Lassa perd',
'qbedit'         => 'Redatá',
'qbspecialpages' => 'Pagin specjaal',
'mytalk'         => 'i mè discüssiun',
'navigation'     => 'Navegá',

'returnto'         => 'Turna indré a $1.',
'help'             => 'Pàgin da jütt',
'search'           => 'Cerca',
'searchbutton'     => 'Cerca',
'go'               => 'Innanz',
'searcharticle'    => 'Và',
'history'          => 'Crunulugia de la pagina',
'history_short'    => 'Crunulugia',
'printableversion' => 'Versiun də stampà',
'permalink'        => 'Culegament permanent',
'edit'             => 'Mudifica',
'editthispage'     => 'Mudifica cula pagina chi',
'create-this-page' => 'Crea cula pagina chi',
'delete'           => 'Scancela',
'undelete_short'   => 'Rimett a post {{PLURAL:$1|1 mudifica|$1 mudifich}}',
'protect'          => 'Bloca',
'unprotect'        => 'sbloca',
'newpage'          => 'Pagina növa',
'talkpagelinktext' => 'ciciarada',
'specialpage'      => 'Pagina speciala',
'talk'             => 'Discüssiun',
'toolbox'          => 'Strüment',
'viewtalkpage'     => 'Varda i discüssiun',
'otherlanguages'   => 'Oltri leench-Otre lengue',
'redirectedfrom'   => '(Rimandaa də $1)',
'jumptonavigation' => 'navegá',
'jumptosearch'     => 'truvá',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'A prupòsit də {{SITENAME}}',
'copyright'            => 'Tücc i cuntegnüü inn dispunibil cuma $1.',
'currentevents'        => 'Atüalitaa',
'currentevents-url'    => 'Project:Avenimeent Receent',
'disclaimers'          => 'Esclüsiun da respunsabilitaa',
'edithelp'             => 'Jütt',
'faq'                  => 'FAQ - Fera Ai Question',
'helppage'             => 'Help:Contegnüü',
'mainpage'             => 'Pagina prinçipala',
'mainpage-description' => 'Pagina prinçipala',
'portal'               => 'Purtaal da cumünitaa',
'portal-url'           => 'Project:Purtaal da cumünitaa',
'privacy'              => "Pulitica de la ''privacy''",

'retrievedfrom'           => 'Utegnüü da "$1"',
'youhavenewmessages'      => "Gh'hinn di $1 ($2).",
'newmessageslink'         => 'messacc nööf',
'newmessagesdifflink'     => 'diferenza par rapòort a la versiun da prima',
'youhavenewmessagesmulti' => "Te gh'è di messagg növ ins'el $1",
'editsection'             => 'Mudifica',
'editold'                 => 'edita',
'toc'                     => 'Cuntegnüü',
'showtoc'                 => 'varda',
'hidetoc'                 => 'scuunt',
'thisisdeleted'           => 'Varda o rimett a pòst $1?',
'restorelink'             => '{{PLURAL:$1|1 mudifica scancelada|$1 mudifich scancelaa}}',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Vus',
'nstab-user'      => 'Pagina persunala',
'nstab-special'   => 'Specjaal',
'nstab-project'   => 'Pagina',
'nstab-mediawiki' => 'Messácc',
'nstab-template'  => 'Bueta',
'nstab-category'  => 'Categuría',

# General errors
'internalerror'     => 'Erur in del sistema',
'badtitle'          => 'Títul mía bun',
'viewsource'        => 'Còdas surgeent',
'protectedpagetext' => "Cula pagina chi l'è stata blucà per impedinn la mudifica.",
'viewsourcetext'    => "L'è pussibil vèd e cupià el codes surgent de cula pagina chí:",
'editinginterface'  => "'''Attenzion''': el testo de quella pagina chì el fà part de l'interfacia utent del sitt. Tutt i modifigh che te fet se vedaran subit su i messagg visualizzaa per tutt i utent.",

# Login and logout pages
'logouttext'              => "<strong>Adess a seis descunetacc.</strong><br />
A podé tirar innanz a dovrar la {{SITENAME}} in manera anònima, a podé
sa cunèta amò cont l'istess o un olt nomm. Tegné cunt che di
pagini i podressa vess fadi vider compagn che a saressov amò conetacc, fin coura che
a scancelé mia la memòria cava dal vost bigat.",
'welcomecreation'         => "== Benvegnüü, $1! ==
Ul tò permèss d'entrava l è prunt. Dismentéga mia de mudifigá i prefereenz dala {{SITENAME}}.",
'yourname'                => 'Nomm ütent',
'yourpassword'            => "Parola d'urdin",
'yourpasswordagain'       => "Mett dent ammò la parola d'urdin",
'remembermypassword'      => "Regordass la mè parola d'urdin",
'nav-login-createaccount' => 'Vena drenta / Crea un cünt',
'loginprompt'             => 'Par cunett a {{SITENAME}}, a duvii abilitá i galet.',
'userlogin'               => 'Entra/Crea un cünt',
'logout'                  => 'Và fö',
'userlogout'              => 'Và fö',
'nologin'                 => 'Sii-f gnamò registraa? $1.',
'nologinlink'             => 'Creé un cüünt!',
'createaccount'           => 'Creá un cüünt',
'createaccountmail'       => 'par adressa da cureel (e-mail)',
'youremail'               => 'E-mail',
'username'                => 'Nomm registraa:',
'yourrealname'            => 'Nomm:',
'yourlanguage'            => 'Lengua:',
'yournick'                => 'Suranomm:',
'email'                   => 'Indirizz de pòsta elettrònica.',
'prefs-help-email'        => 'Courriel (e-mail) al é mia obligatòri, però al permet da va mandar una nœuva ciav se cas che va la desmenteghé. A podé apó scernir da lassar i olt dovracc entrar en contat con violt senza da busogn da svelar la vosta identitaa.',
'noname'                  => "Vüü avii mía specificaa un nomm d'üsüari valévul.",
'loginsuccesstitle'       => "La cunessiun l'è scumenzada cun sücess.",
'loginsuccess'            => 'Te set cuness a {{SITENAME}} cume "$1".',
'mailmypassword'          => "Desmentegaa la parola d'urdin?",
'emailauthenticated'      => 'Ul tò adrèss e-mail l è staa verificaa: $1.',
'emailnotauthenticated'   => 'Ul tò adrèss da pòsta letronica l è mia staa gnamò verificaa. Nissün mesacc al saraa mandaa par i servizzi che segütan.',
'accountcreated'          => 'Cunt bell-e-cread',

# Edit pages
'summary'              => 'Argument de la mudifica',
'minoredit'            => "Chesta chi l'è una mudifica da impurtanza minuur",
'watchthis'            => "Tegn d'öcc questa pagina",
'savearticle'          => 'Salva',
'preview'              => 'Varda prima de salvà la pagina',
'showpreview'          => 'Famm vedè prima',
'showdiff'             => 'Famm vedè i cambiament',
'anoneditwarning'      => 'Tì te set minga entraa. In de la crunulugia de la pagina se vedarà el tò IP.',
'accmailtext'          => 'La parola d\'urdin per "$1" l\'è stada mandada a $2.',
'anontalkpagetext'     => "----''Questa chì l'è la pagina de discüssiun de un ütent che l'ha minga ammò registraa un cünt, upür che el vör minga duperàll; dunca, el pò vess identificaa dumà cunt el sò IP, ch'el pò vess spartii tra tanti ütent diferent. Se ti te set un ütent anonim e t'hee vist un quai messacc ch'el te par ch'el gh'entra nagott cun tì, pröva a [[Special:UserLogin|creà el tò cünt]] per fà pü casott.''",
'noarticletext'        => "Gh'è minga del test in quella pagina chì. Te pòdet [[Special:Search/{{PAGENAME}}|cercà in d'on'altra pagina]] oppur [{{fullurl:{{FULLPAGENAME}}|action=edit}} creàla tì].",
'clearyourcache'       => "'''Nòta:''' dòpu che avii salvaa, pudaría véss neçessari de scancelá la memòria \"cache\" dal vòst prugráma də navigazziún in reet par vidé i mudifich faa. '''Mozilla / Firefox / Safari:''' tegní schiscjaa al butún ''Shift'' intaant che sə clica ''Reload'', upüür schiscjá ''Ctrl-Shift-R'' (''Cmd-Shift-R'' sül Apple Mac); '''IE:''' schiscjá ''Ctrl'' intaant che sə clica ''Refresh'', upüür schiscjá ''Ctrl-F5''; '''Konqueror:''': semplicemeent clicá al butún ''Reload'', upüür schiscjá ''F5''; '''Opera''' i üteent pudarían vech büsögn da scancelá cumpletameent la memòria \"cache\" in ''Tools&rarr;Preferences''.",
'previewnote'          => "<strong>'''Atenziun'''! Questa pagina la serviss dumà de vardà. I cambiament hinn minga staa salvaa.</strong>",
'editing'              => 'Mudifica de $1',
'editingcomment'       => 'Redataant $1 (cumentari)',
'yourtext'             => 'El tò test',
'yourdiff'             => 'Diferenzi',
'protectedpagewarning' => '<strong>ATENZIÚN: chésta pagina l è staja blucava in manéra che dumá i üteent cunt i privilegi də sysop a pòdan mudificala.</strong>',
'templatesused'        => 'Buete duvrade in chesta pàgina - Buett duvraat in chesta pàgina:',

# History pages
'next'       => 'pròssim',
'last'       => 'ültima',
'histlegend' => "Cercá i difəreenz: selezziuná i balitt di versiún de cumpará e pö schiscjá ''enter'' upüür al butún in scima ala tabèlina.<br />
Spiegazziún di símbui: (cur) = difərenza cun la versiún curénta, (ültima) = difərenza cun l'ültima versiún, M = redatazziún də impurtanza minuur.",
'histfirst'  => 'Püssee vecc',
'histlast'   => 'Püssee receent',

# Diffs
'compareselectedversions' => 'Cumpara i versiun selezziunaa',

# Search results
'noexactmatch'          => "'''La pagina \"\$1\" la esista no.''' L'è pussibil [[:\$1|creala adèss]].",
'noexactmatch-nocreate' => "'''La pagina cun el titul \"\$1\" la esista no.'''",
'toomanymatches'        => "Gh'è tropi curispundens. Mudifichè la richiesta.",
'prevn'                 => 'preçedeent $1',
'nextn'                 => 'pròssim $1',
'viewprevnext'          => 'Vidé ($1) ($2) ($3).',
'powersearch'           => 'Truvá',

# Preferences page
'preferences'        => 'Prefereenz',
'mypreferences'      => 'i mè prefereenz',
'changepassword'     => "Mudifega la paròla d'urdin",
'skin'               => "Aspett de l'interfacia",
'math'               => 'Matem',
'dateformat'         => 'Furmaa da la data',
'datedefault'        => 'Nissüna preferenza',
'datetime'           => 'Data e urari',
'prefs-personal'     => 'Carateristich dal üteent',
'prefs-rc'           => 'Cambiameent reçeent',
'prefs-misc'         => 'Vari',
'saveprefs'          => 'Tegn i mudifech',
'resetprefs'         => 'Trá via i mudifech',
'oldpassword'        => "Paròla d'urdin végja:",
'newpassword'        => "Paròla d'urdin növa:",
'retypenew'          => "Scriif ancamò la paròla d'urdin növa:",
'textboxsize'        => 'Mudifich',
'rows'               => 'Riich:',
'columns'            => 'Culònn:',
'searchresultshead'  => 'Cerca',
'resultsperpage'     => 'Resültaa pər pagina:',
'contextlines'       => 'Riich pər resültaa:',
'contextchars'       => 'Cuntèst pər riga:',
'recentchangescount' => 'Titui in di "cambiameent reçeent":',
'savedprefs'         => 'I prefereenz in stai salvaa.',
'timezonelegend'     => 'Lucalitaa',
'timezonetext'       => 'I uur da diferenza tra l urari lucaal e chél dal sèrver (UTC).',
'localtime'          => 'Urari lucaal',
'timezoneoffset'     => 'Diferenza¹',
'servertime'         => 'Urari dal sèrver',
'guesstimezone'      => 'Catá l urari dal sèrver',
'allowemail'         => 'Permètt ai altar üteent də cuntatamm par email',
'defaultns'          => 'Tröva sempar in di caamp:',
'files'              => 'Archivi',

# User rights
'userrights-lookup-user'   => 'Gestione dei gruppi utente',
'userrights-user-editname' => 'Inserire il nome utente:',
'editusergroup'            => 'Modifica gruppi utente',
'userrights-editusergroup' => 'Modifica gruppi utente',
'saveusergroups'           => 'Salva gruppi utente',
'userrights-groupsmember'  => 'Appartiene ai gruppi:',
'userrights-reason'        => 'Motivo della modifica:',
'userrights-no-interwiki'  => 'Non si dispone dei permessi necessari per modificare i diritti degli utenti su altri siti.',
'userrights-nodatabase'    => 'Il database $1 non esiste o non è un database locale.',
'userrights-nologin'       => "Per assegnare diritti agli utenti è necessario [[Special:UserLogin|effettuare l'accesso]] come amministratore.",
'userrights-notallowed'    => "L'utente non dispone dei permessi necessari per assegnare diritti agli utenti.",

# Groups
'group-user' => 'Dovracc',

'group-user-member' => 'Dovratt',

'grouppage-user' => '{{ns:project}}:Dovracc',

# Rights
'right-edit'          => 'Edita pàgini',
'right-createaccount' => 'Crea di nouvel cunt de dovratt',

# Recent changes
'recentchanges'     => 'Cambiameent reçeent',
'recentchangestext' => 'In chesta pagina a inn evidenziaa i cambiameent püssee receent al wiki lumbaart.',
'rclistfrom'        => 'Fá vidé i nööf cambiameent a partí də $1',
'rcshowhideminor'   => '$1 mudifich menu impurtaant',
'rcshowhideliu'     => '$1 üteent cunèss',
'rcshowhideanons'   => '$1 üteent anònim',
'rcshowhidemine'    => '$1 i mè mudifich',
'rclinks'           => 'Fá vidé i ültim $1 cambiameent indi ültim $2 dí<br />$3',
'diff'              => 'dif',
'hist'              => 'stòria',
'hide'              => 'Scuunt',
'show'              => 'Famm vedè',

# Recent changes linked
'recentchangeslinked' => 'Cambiament culegaa',

# Upload
'upload'            => 'Carga sü un file',
'uploadbtn'         => 'Carga sü',
'uploadnologin'     => 'Minga cuness',
'filedesc'          => 'Sumari',
'fileuploadsummary' => 'Sumari:',
'ignorewarnings'    => 'Ignora tücc i avertimeent',
'largefileserver'   => 'Chel archivi-chí al è püssee graant che ul serviduur al sía cunfigüraa da permett.',
'sourcefilename'    => "Nomm da l'archivi surgeent:",
'destfilename'      => "Nomm da l'archivi da destinazziun:",

# Special:ImageList
'imgfile'        => 'archivi',
'imagelist'      => 'Listá i imàgin',
'imagelist_date' => 'Dada',
'imagelist_name' => 'Nomm',
'imagelist_user' => 'Dovratt',

# Image description page
'filehist-revert' => "Butar torna 'me ch'al era",
'imagelinks'      => 'Ligámm',

# MIME search
'mimesearch' => 'cérca MIME',

# Unwatched pages
'unwatchedpages' => "Pagin mia tegnüü d'öcc",

# List redirects
'listredirects' => 'Listá i pagin re-indirizzaa',

# Unused templates
'unusedtemplates' => 'Templat mia druvaa',

# Random page
'randompage' => 'Página a caas',

# Statistics
'statistics' => 'Statistich',
'userstats'  => 'Statistich di utent',

'disambiguations' => 'Pagin da disambiguazziún',

'doubleredirects' => 'Redirezziún dópi',

'brokenredirects' => 'Redirezziún interótt',

# Miscellaneous special pages
'uncategorizedpages'      => 'Pagin mia categurizzaa',
'uncategorizedcategories' => 'Categurii mia categurizzaa',
'unusedcategories'        => 'Categurii mia druvaa',
'unusedimages'            => 'Imagin mia druvaa',
'wantedcategories'        => 'Categurii ricercaa',
'wantedpages'             => 'Pagin ricercaa',
'mostlinked'              => 'Püssè ligaa a pagin',
'mostlinkedcategories'    => 'Püssè ligaa ai categurii',
'mostcategories'          => 'Articui cun püssè categurii',
'mostimages'              => 'Püssè ligaa a imagin',
'mostrevisions'           => 'Articui cun püssè revisiún',
'prefixindex'             => 'Pagin cul nóm che cumencja par...',
'shortpages'              => 'Pagin püssee curt',
'longpages'               => 'Pagin püssè luunch',
'deadendpages'            => 'Pagin senza surtida',
'listusers'               => 'Listá i üteent registraa',
'newpages'                => 'Pagin nööf',
'ancientpages'            => 'Pagin püssee vecc',

# Book sources
'booksources' => 'Surgeent librari',

# Special:Log
'specialloguserlabel'  => 'Üteent:',
'speciallogtitlelabel' => 'Titul:',
'logempty'             => "El log l'è vöj.",

# Special:AllPages
'allpages'       => 'Tücc i pagin',
'allpagesfrom'   => 'Famm vedè i pagin a partì de:',
'allarticles'    => 'Tütt i vus',
'allpagesprev'   => 'Precedent',
'allpagesnext'   => 'Pròssim',
'allpagessubmit' => 'Innanz',
'allpagesprefix' => "Varda i pagin ch'i scumenza per:",

# Special:Categories
'categories' => 'Categurii',

# E-mail user
'emailuser' => 'Manda un email al duvrátt',

# Watchlist
'watchlist'        => 'In usservazziun',
'addedwatch'       => "Gjüntaa a la lista dii pagin də tegn d'öcc",
'addedwatchtext'   => "La pagina \"[[:\$1]]\" l'è staja gjüntava a la lista dii [[Special:Watchlist|paginn da tegn d'öcc]].
I cambiameent che i vegnará fai a chesta pagina chi e a la sóa pagina dii cumünicazziún
i vegnará segnalaa chichinscí e la pagina la sa vedará cun caráter '''spèss''' in la
[[Special:RecentChanges|lista dii cambiameent reçeent]] gjüst par evidenziála.
<p>Se ti vörat tirá via chesta pagina chi dala lista dai paginn da tegn d'öcc ti pòdat schiscjá
al butún \"tegn piü d'öcc\".",
'removedwatch'     => 'Scancelaa dala lista di usservazziún.',
'removedwatchtext' => 'La pagina "[[:$1]]" l\'è staja scancelava dala tóa lista da usservazziún.',
'watch'            => "Tegn d'öcc",
'watchthispage'    => "Tegn d'öcc questa pagina",
'unwatch'          => "Tegn pü d'öcc",
'watchnochange'    => "Nissün cambiameent l è stai faa süi articui/págin che ti tegnat d'öcc indal períut da teemp selezziunaa.",
'wlshowlast'       => 'Fa vidé i ültim $1 uur $2 dí $3',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => "Giuntà ai pagin da ten d'ögg...",
'unwatching' => "Eliminà dai pagin da ten d'ögg...",

'enotif_newpagetext' => "Chesta-chí l'è una pàgina növa.",
'changed'            => 'cambiaa',

# Delete/protect/revert
'deletepage'            => 'Scancela la pagina',
'historywarning'        => "Atenziún: La pagina che a sii dré a scancelá la gh'a una stòria:",
'actioncomplete'        => 'Aziun cumpletada',
'deletedtext'           => 'La pagina "<nowiki>$1</nowiki>" l\'è stada scancelada. Varda el $2 per una lista di ültim scancelaziun.',
'deletionlog'           => 'log di scancelaziun',
'deletecomment'         => 'Mutiif dala scancelazziun',
'deleteotherreason'     => 'Alter mutiv:',
'deletereason-dropdown' => "*Mutiv cumün de scancelaziun
** Richiesta de l'aütur
** Viulaziun del copyright
** Vandalism",
'rollback'              => 'Rollback',
'rollbacklink'          => 'Rollback',
'rollbackfailed'        => 'L è mia staa pussibil purtá indré',
'alreadyrolled'         => "L è mia pussibil turná indré al'ültima versiún da [[:$1]] dal [[User:$2|$2]] ([[User talk:$2|Discüssiún]]); un quaivün l á gjamò redataa o giraa indré la pagina.
L'ültima redatazziún l eva da [[User:$3|$3]] ([[User talk:$3|Discüssiún]]).",
'unprotectedarticle'    => 'l\'ha sblucaa "[[$1]]"',
'protect-title'         => 'Prutezziún da "$1"',
'protect-legend'        => 'Cunferma de blocch',
'protectcomment'        => 'Spiega parchè ti vörat blucá la pagina',

# Undelete
'undelete'           => 'Varda i pagin scancelaa',
'undelete-nodiff'    => "Per questa pagina gh'è nanca una revisiun precedenta.",
'undeletebtn'        => 'Rimett a post',
'undeletedarticle'   => 'rimetüü a post "[[$1]]"',
'undeletedrevisions' => '{{PLURAL:$1|1 revision|$1 versiun}} rimetüü a post',

# Namespace form on various pages
'invert'         => 'Invertí la selezziún',
'blanknamespace' => '(Principal)',

# Contributions
'contributions' => 'Cuntribüzziún dal duvratt',
'mycontris'     => 'I mè interveent',
'uctop'         => '(ültima per la pagina)',

# What links here
'whatlinkshere' => 'Pagin che se culeghen chì',

# Block/unblock
'blockip'       => "Bloca l'ütent",
'ipblocklist'   => 'Listá i adrèss IP e i üteent blucaa',
'blocklistline' => "$1, $2 l'ha blucaa $3 ($4)",
'blocklink'     => 'bloca',
'contribslink'  => 'cuntribüzziún',
'blocklogpage'  => 'Log di blocch',
'blocklogentry' => "l'ha blucaa [[$1]] per un temp de $2 $3",

# Move page
'movepagetext'    => "Duvraant la büeta chí-da-sota al re-numinerà una pàgina, muveent tüta la suva stòria al nomm nööf. Ul vecc títul al deventarà una pàgina da redirezziun al nööf títul. I liamm a la vegja pàgina i sarà mia cambiaa: assürévas da cuntrulá par redirezziun dopi u rumpüüt.
A sii respunsàbil da assüráss che i liamm i sigüta a puntá intúe i è süpunüü da ná.
Nutii che la pàgina la sarà '''mia''' muvüda se a gh'è gjamò una pàgina al nööf títul, a maanch che la sía vöja, una redirezziun cun nissüna stòtia d'esizziun passada. Cheest-chí al signífega ch'a pudii renuminá indrée
una pàgina intúe l'évuf renuminada via par eruur, e che vüü pudii mia surascriif una pàgina esisteent.


<b>ATENZIUN!</b>
Cheest-chí al pöö vess un canbi dràstegh e inaspetaa par una pàgina pupülara: par piasée assürévas ch'a ii capii i cunsegueenz da cheest-chí prima da ná inaanz.",
'movedto'         => 'spustaa vers:',
'1movedto2'       => '[[$1]] spustaa in [[$2]]',
'1movedto2_redir' => '[[$1]] spustaa in [[$2]] atravèerz re-indirizzameent',
'delete_and_move' => 'Scancelá e mööf',

# Export
'export' => 'Espurtá pagin',

# Namespace 8 related
'allmessages'         => 'Tücc i messacc dal sistéma',
'allmessagesdefault'  => 'Test standard',
'allmessagescurrent'  => 'Test curent',
'allmessagestext'     => 'Chesta chí l è una lista də messácc də sistema dispunibil indal MediaWiki: namespace.',
'allmessagesfilter'   => 'Varda dumà i messacc che tegnen dent:',
'allmessagesmodified' => 'Varda dumá i messacc mudificaa',

# Thumbnails
'thumbnail-more' => 'Ingrandí',

# Special:Import
'import' => 'Impurtá di pagin',

# Tooltip help for the actions
'tooltip-ca-addsection'           => 'Taca un cument a questa discüssiun',
'tooltip-ca-delete'               => 'Scancela questa pagina',
'tooltip-n-mainpage'              => 'Visité la pàgina principala',
'tooltip-n-portal'                => "Descripzion del proget, cossa ch'a podé far, dond trovar vergòt",
'tooltip-n-currentevents'         => "Informazion ansima a vergòt ch'al riva.",
'tooltip-n-recentchanges'         => 'Lista de canviamenc recenc del wiki',
'tooltip-n-randompage'            => "Càrrega una pàgina a l'azard",
'tooltip-n-help'                  => "Pàgini d'aida",
'tooltip-t-whatlinkshere'         => "Lista de tuti li pàgini wiki ch'i liga scià",
'tooltip-t-recentchangeslinked'   => 'Canviamenc recenc en li pàgini ligadi a chesta',
'tooltip-feed-rss'                => 'Feed RSS per chesta pàgina',
'tooltip-t-specialpages'          => 'Lista de tütt i pagin speciaal',
'tooltip-compareselectedversions' => 'Far vider li diferenzi entra li doi version selezionadi da chesta pàgina',

# Attribution
'siteuser' => '{{SITENAME}} ütent $1',

# Math options
'mw_math_png'    => 'Trasfurmá sempər in PNG',
'mw_math_simple' => 'HTML se mia cumplicaa altrimeent PNG',
'mw_math_html'   => 'HTML se l è pussíbil altrimeent PNG',
'mw_math_source' => 'Lassá in furmaa TeX (pər i prugráma də navigazziún dumá in furmaa da testu)',
'mw_math_modern' => 'Racumandaa pər i bigatt püssè reçeent',
'mw_math_mathml' => 'MathML se l è pussíbil (sperimentaal)',

# Media information
'imagemaxsize' => 'Limitá i imagin süi pagin da descrizziún dii imagin a:',
'thumbsize'    => 'Dimensiún diapusitiif:',

# Special:NewImages
'newimages' => 'Espusizziun di imàgin nööf',
'ilsubmit'  => 'Truvá',

# External editor support
'edit-externally'      => 'Redatá chest archivi cunt un prugramari da fö',
'edit-externally-help' => 'Vidé i [http://www.mediawiki.org/wiki/Manual:External_editors istrüzziún] pər vech püssè infurmazziún (in Inglees).',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'tücc',
'imagelistall'     => 'tücc',
'watchlistall2'    => 'tücc',
'namespacesall'    => 'tücc',

# E-mail address confirmation
'confirmemail'          => "Cunferma l<nowiki>'</nowiki>''e-mail''",
'confirmemail_text'     => "Prima da pudé riçeef mesacc sül tò adrèss da pòsta letrònica l è neçessari verificál.
Schiscjá ul butún che gh'è chi da sót par curfermá al tò adrèss.
Te riçevaree un mesacc cun deent un ligamm specjal; ti duvaree clicaa sül ligamm par cunfermá che l tò adrèss l è válit.",
'confirmemail_send'     => 'Mandum un mesacc da cunfermazziún',
'confirmemail_sent'     => 'Ul mesacc da cunfermazziún l è staa mandaa.',
'confirmemail_success'  => "La Vostra adressa cureel l'è stada cunfermada: adess vüü pudii duvrá ul wiki",
'confirmemail_loggedin' => "Adess la vostra adressa da cureel (e-mail) l'è stada cunfermada",

# Auto-summaries
'autosumm-blank' => 'Pagina svujada',

# Special:Version
'version' => 'Versiun', # Not used as normal message but as header for the special page itself

# Special:FilePath
'filepath' => 'Percuurz daj archivi',

# Special:SpecialPages
'specialpages' => 'Pagin special',

);
