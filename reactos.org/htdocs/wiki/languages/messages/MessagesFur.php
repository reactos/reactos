<?php
/** Friulian (Furlan)
 *
 * @ingroup Language
 * @file
 *
 * @author Helix84
 * @author Klenje
 * @author לערי ריינהארט
 */

$fallback = 'it';

$skinNames = array(
	'standard'    => 'Classiche',
	'nostalgia'   => 'Nostalgjie',
	'modern'      => 'Moderne',
);

$namespaceNames = array(
	NS_MEDIA          => 'Media',
	NS_SPECIAL        => 'Speciâl',
	NS_MAIN           => '',
	NS_TALK           => 'Discussion',
	NS_USER           => 'Utent',
	NS_USER_TALK      => 'Discussion_utent',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => 'Discussion_$1',
	NS_IMAGE          => 'Figure',
	NS_IMAGE_TALK     => 'Discussion_figure',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'Discussion_MediaWiki',
	NS_TEMPLATE       => 'Model',
	NS_TEMPLATE_TALK  => 'Discussion_model',
	NS_HELP	          => 'Jutori',
	NS_HELP_TALK      => 'Discussion_jutori',
	NS_CATEGORY       => 'Categorie',
	NS_CATEGORY_TALK  => 'Discussion_categorie'
);

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'ReIndreçamentsDoplis' ),
	'BrokenRedirects'           => array( 'ReIndreçamentsSbaliâts' ),
	'Disambiguations'           => array( 'Omonimiis' ),
	'Userlogin'                 => array( 'Jentre', 'Login' ),
	'Userlogout'                => array( 'Jes', 'Logout' ),
	'CreateAccount'             => array( 'CreeIdentitât' ),
	'Preferences'               => array( 'Preferencis' ),
	'Watchlist'                 => array( 'TignudisDiVoli' ),
	'Recentchanges'             => array( 'UltinsCambiaments' ),
	'Upload'                    => array( 'Cjame' ),
	'Imagelist'                 => array( 'Figuris' ),
	'Newimages'                 => array( 'GnovisFiguris' ),
	'Listusers'                 => array( 'Utents', 'ListeUtents' ),
	'Statistics'                => array( 'Statistichis' ),
	'Randompage'                => array( 'PagjineCasuâl' ),
	'Lonelypages'               => array( 'PagjinisSolitariis' ),
	'Uncategorizedpages'        => array( 'PagjinisCenceCategorie' ),
	'Uncategorizedcategories'   => array( 'CategoriisCenceCategorie' ),
	'Uncategorizedimages'       => array( 'FigurisCenceCategorie' ),
	'Uncategorizedtemplates'    => array( 'ModeiCenceCategorie' ),
	'Unusedcategories'          => array( 'CategoriisNoDopradis' ),
	'Unusedimages'              => array( 'FigurisNoDopradis' ),
	'Wantedcategories'          => array( 'CategoriisDesideradis' ),
	'Shortpages'                => array( 'PagjinisPluiCurtis' ),
	'Longpages'                 => array( 'PagjinisPluiLungjis' ),
	'Newpages'                  => array( 'GnovisPagjinis' ),
	'Ancientpages'              => array( 'PagjinisPluiVieris' ),
	'Deadendpages'              => array( 'PagjinisCenceJessude' ),
	'Protectedpages'            => array( 'PagjinisProtezudis' ),
	'Protectedtitles'           => array( 'TituiProtezûts' ),
	'Allpages'                  => array( 'DutisLisPagjinis' ),
	'Prefixindex'               => array( 'Prefìs' ),
	'Ipblocklist'               => array( 'IPBlocâts' ),
	'Specialpages'              => array( 'PagjinisSpeciâls' ),
	'Contributions'             => array( 'Contribûts', 'ContribûtsUtent' ),
	'Emailuser'                 => array( 'MandeEmail' ),
	'Confirmemail'              => array( 'ConfermePuesteEletroniche' ),
	'Whatlinkshere'             => array( 'Leams' ),
	'Recentchangeslinked'       => array( 'CambiamentsLeâts' ),
	'Movepage'                  => array( 'Môf', 'CambieNon' ),
	'Booksources'               => array( 'RicercjeISBN' ),
	'Categories'                => array( 'Categoriis' ),
	'Export'                    => array( 'Espuarte' ),
	'Version'                   => array( 'Version' ),
	'Allmessages'               => array( 'Messaçs' ),
	'Log'                       => array( 'Regjistri', 'Regjistris' ),
	'Blockip'                   => array( 'BlocheIP' ),
	'Undelete'                  => array( 'Ripristine' ),
	'Import'                    => array( 'Impuarte' ),
	'Lockdb'                    => array( 'BlocheDB' ),
	'Unlockdb'                  => array( 'SblocheDB' ),
	'Userrights'                => array( 'PermèsUtents' ),
	'MIMEsearch'                => array( 'RicercjeMIME' ),
	'Unwatchedpages'            => array( 'PagjinisNoTignudisDiVoli' ),
	'Listredirects'             => array( 'ListeReIndreçaments' ),
	'Revisiondelete'            => array( 'ScanceleRevision' ),
	'Unusedtemplates'           => array( 'ModeiNoDoprâts' ),
	'Randomredirect'            => array( 'ReIndreçamentCasuâl' ),
	'Mypage'                    => array( 'MêPagjineUtent' ),
	'Mytalk'                    => array( 'MêsDiscussions' ),
	'Mycontributions'           => array( 'MieiContribûts' ),
	'Listadmins'                => array( 'ListeAministradôrs' ),
	'Listbots'                  => array( 'ListeBots' ),
	'Popularpages'              => array( 'PagjinisPopolârs' ),
	'Search'                    => array( 'Ricercje', 'Cîr' ),
	'Resetpass'                 => array( 'ReimpuestePerauleClâf' ),
	'Withoutinterwiki'          => array( 'CenceInterwiki' ),
);

$datePreferences = false;
$defaultDateFormat = 'dmy';
$dateFormats = array(
	'dmy time' => 'H:i',
	'dmy date' => 'j "di" M Y',
	'dmy both' => 'j "di" M Y "a lis" H:i',
);

$separatorTransformTable = array(',' => "\xc2\xa0", '.' => ',' );

$messages = array(
# User preference toggles
'tog-underline'               => 'Sotlinee leams',
'tog-highlightbroken'         => 'Mostre leams sbaliâts <a href="" class="new">cussì</a> (invezit di cussì<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Justifiche paragraf',
'tog-hideminor'               => 'Plate lis piçulis modifichis tai ultins cambiaments',
'tog-usenewrc'                => 'Ultins cambiaments avanzâts (JavaScript)',
'tog-numberheadings'          => 'Numerazion automatiche dai titui',
'tog-showtoolbar'             => 'Mostre sbare dai imprescj pe modifiche (JavaScript)',
'tog-editondblclick'          => 'Cambie lis pagjinis fracant dôs voltis (JavaScript)',
'tog-editsection'             => 'Inserìs un leam [cambie] pe editazion veloç di une sezion',
'tog-editsectiononrightclick' => 'Modifiche une sezion fracant cul tast diestri<br /> sui titui des sezions (JavaScript)',
'tog-showtoc'                 => 'Mostre la tabele dai contignûts pes pagjinis cun plui di 3 sezions',
'tog-rememberpassword'        => 'Visiti tes prossimis sessions',
'tog-editwidth'               => 'Il spazi pe modifiche al è larc il plui pussibil',
'tog-watchdefault'            => 'Zonte in automatic lis pagjinis che o cambii inte liste di chês tignudis di voli',
'tog-minordefault'            => 'Imposte come opzion predeterminade ducj i cambiaments come piçui',
'tog-previewontop'            => 'Mostre anteprime parsore dal spazi pe modifiche',
'tog-previewonfirst'          => 'Mostre anteprime te prime modifiche',
'tog-nocache'                 => 'No stâ tignî in memorie (caching) lis pagjinis',
'tog-enotifwatchlistpages'    => 'Mandimi une email se la pagjine e gambie',
'tog-enotifusertalkpages'     => 'Mandimi une email cuant che la mê pagjine di discussion e gambie',
'tog-enotifminoredits'        => 'Mandimi une email ancje pai piçui cambiaments ae pagjine',
'tog-enotifrevealaddr'        => 'Distapone fûr il gno recapit email tai messaçs di notifiche',
'tog-shownumberswatching'     => 'Mostre il numar di utents che a stan tignint di voli',
'tog-fancysig'                => 'Firmis crudis (cence leam automatic)',
'tog-externaleditor'          => 'Dopre editôr esterni come opzion predeterminade',
'tog-externaldiff'            => 'Dopre editôr difarencis esterni come opzion predeterminade',
'tog-watchlisthideown'        => 'Plate i miei cambiaments inte liste des pagjinis tignudis di voli',
'tog-ccmeonemails'            => 'Mandimi une copie dai messaçs che o mandi ai altris utents',
'tog-showhiddencats'          => 'Mostre categoriis platadis',

'underline-always'  => 'Simpri',
'underline-never'   => 'Mai',
'underline-default' => 'Predeterminât dal sgarfadôr',

'skinpreview' => '(Anteprime)',

# Dates
'sunday'        => 'Domenie',
'monday'        => 'Lunis',
'tuesday'       => 'Martars',
'wednesday'     => 'Miercus',
'thursday'      => 'Joibe',
'friday'        => 'Vinars',
'saturday'      => 'Sabide',
'sun'           => 'dom',
'mon'           => 'lun',
'tue'           => 'mar',
'wed'           => 'mie',
'thu'           => 'joi',
'fri'           => 'vin',
'sat'           => 'sab',
'january'       => 'Zenâr',
'february'      => 'Fevrâr',
'march'         => 'Març',
'april'         => 'Avrîl',
'may_long'      => 'Mai',
'june'          => 'Jugn',
'july'          => 'Lui',
'august'        => 'Avost',
'september'     => 'Setembar',
'october'       => 'Otubar',
'november'      => 'Novembar',
'december'      => 'Dicembar',
'january-gen'   => 'Zenâr',
'february-gen'  => 'Fevrâr',
'march-gen'     => 'Març',
'april-gen'     => 'Avrîl',
'may-gen'       => 'Mai',
'june-gen'      => 'Jugn',
'july-gen'      => 'Lui',
'august-gen'    => 'Avost',
'september-gen' => 'Setembar',
'october-gen'   => 'Otubar',
'november-gen'  => 'Novembar',
'december-gen'  => 'Dicembar',
'jan'           => 'Zen',
'feb'           => 'Fev',
'mar'           => 'Mar',
'apr'           => 'Avr',
'may'           => 'Mai',
'jun'           => 'Jug',
'jul'           => 'Lui',
'aug'           => 'Avo',
'sep'           => 'Set',
'oct'           => 'Otu',
'nov'           => 'Nov',
'dec'           => 'Dic',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Categorie|Categoriis}}',
'category_header'                => 'Vôs inte categorie "$1"',
'subcategories'                  => 'Sot categoriis',
'category-media-header'          => 'Files inte categorie "$1"',
'category-empty'                 => "''Par cumò la categorie no conten ni pagjinis ni files multimediâi.''",
'hidden-categories'              => '{{PLURAL:$1|Categorie platade|Categoriis platadis}}',
'hidden-category-category'       => 'Categoriis platadis', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Cheste categorie e conten une sot categorie, mostrade ca sot.|Cheste categorie e conten {{PLURAL:$1|la sot categorie|lis $1 sot categoriis}} ca sot suntun totâl di $2.}}',
'category-subcat-count-limited'  => 'Cheste categorie e conten {{PLURAL:$1|une sot categorie, mostrade|$1 sot categoriis, mostradis}} sot.',
'category-article-count'         => '{{PLURAL:$2|Cheste categorie e conten dome une pagjine mostrade ca sot.|Cheste categorie e conten {{PLURAL:$1|la pagjine indicade|lis $1 pagjinis indicadis}} di seguit, suntun totâl di $2.}}',
'category-article-count-limited' => 'Cheste categorie e conten {{PLURAL:$1|la pagjine|lis $1 pagjinis}} ca sot.',
'category-file-count'            => '{{PLURAL:$2|Cheste categorie e conten dome un file, mostrât ca sot.|Cheste categorie e conten {{PLURAL:$1|un file, mostrât|$1 files, mostrâts}} ca sot, suntun totâl di $2.}}',
'category-file-count-limited'    => 'Cheste categorie e conten {{PLURAL:$1|il file mostrât|i $1 files mostrâts}} ca sot.',
'listingcontinuesabbrev'         => 'cont.',

'mainpagetext' => "'''MediaWiki e je stade instalade cun sucès.'''",

'about'          => 'Informazions',
'article'        => 'Vôs',
'newwindow'      => '(al vierç un gnûf barcon)',
'cancel'         => 'Scancele',
'qbfind'         => 'Cjate',
'qbbrowse'       => 'Sgarfe',
'qbedit'         => 'Cambie',
'qbpageoptions'  => 'Cheste pagjine',
'qbpageinfo'     => 'Contest',
'qbmyoptions'    => 'Mês pagjinis',
'qbspecialpages' => 'Pagjinis speciâls',
'moredotdotdot'  => 'Plui...',
'mypage'         => 'Mê pagjine',
'mytalk'         => 'Mês discussions',
'anontalk'       => 'Discussion par chest IP',
'navigation'     => 'somari',
'and'            => 'e',

'errorpagetitle'    => 'Erôr',
'returnto'          => 'Torne a $1.',
'tagline'           => 'Di {{SITENAME}}',
'help'              => 'Jutori',
'search'            => 'Cîr',
'searchbutton'      => 'Cîr',
'go'                => 'Va',
'searcharticle'     => 'Va',
'history'           => 'Storic de pagjine',
'history_short'     => 'Storic',
'updatedmarker'     => 'inzornât de mê ultime visite',
'info_short'        => 'Informazions',
'printableversion'  => 'Version stampabil',
'permalink'         => 'Leam permanent',
'print'             => 'Stampe',
'edit'              => 'Cambie',
'create'            => 'Cree',
'editthispage'      => 'Cambie cheste pagjine',
'create-this-page'  => 'Cree cheste pagjine',
'delete'            => 'Elimine',
'deletethispage'    => 'Elimine cheste pagjine',
'undelete_short'    => 'Recupere {{PLURAL:$1|modifiche eliminade|$1 modifichis eliminadis}}',
'protect'           => 'Protêç',
'protectthispage'   => 'Protêç cheste pagjine',
'unprotect'         => 'No stâ protezi',
'unprotectthispage' => 'No stâ plui protezi cheste pagjine',
'newpage'           => 'Gnove pagjine',
'talkpage'          => 'Fevelin di cheste pagjine',
'talkpagelinktext'  => 'discussion',
'specialpage'       => 'Pagjine speciâl',
'personaltools'     => 'Imprescj personâi',
'postcomment'       => 'Zonte un coment',
'articlepage'       => 'Cjale la vôs',
'talk'              => 'Discussion',
'views'             => 'Visitis',
'toolbox'           => 'imprescj',
'userpage'          => 'Cjale pagjine dal utent',
'projectpage'       => 'Cjale pagjine dal progjet',
'imagepage'         => 'Cjale pagjine de figure',
'mediawikipage'     => 'Cjale la pagjine dal messaç',
'categorypage'      => 'Cjale la categorie',
'viewtalkpage'      => 'Cjale la pagjine di discussion',
'otherlanguages'    => 'Altris lenghis',
'redirectedfrom'    => '(Inviât ca di $1)',
'redirectpagesub'   => 'Pagjine di redirezion',
'lastmodifiedat'    => "Cambiât par l'ultime volte ai $2, $1", # $1 date, $2 time
'viewcount'         => 'Cheste pagjine e je stade lete {{PLURAL:$1|une volte|$1 voltis}}.',
'protectedpage'     => 'Pagjine protezude',
'jumpto'            => 'Va a:',
'jumptonavigation'  => 'navigazion',
'jumptosearch'      => 'ricercje',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Informazions su {{SITENAME}}',
'aboutpage'            => 'Project:Informazions',
'bugreports'           => 'Segnalazions di malfunzionaments',
'bugreportspage'       => 'Project:Malfunzionaments',
'copyright'            => 'Il contignût al è disponibil sot de $1',
'copyrightpage'        => '{{ns:project}}:Copyrights',
'currentevents'        => 'Lis gnovis',
'currentevents-url'    => 'Project:Lis gnovis',
'disclaimers'          => 'Avîs legâi',
'disclaimerpage'       => 'Project:Avîs gjenerâi',
'edithelp'             => 'Jutori pai cambiaments',
'edithelppage'         => 'Help:Cambiaments',
'helppage'             => 'Help:Contignûts',
'mainpage'             => 'Pagjine principâl',
'mainpage-description' => 'Pagjine principâl',
'portal'               => 'Ostarie',
'portal-url'           => 'Project:Ostarie',
'privacy'              => 'Politiche pe privacy',
'privacypage'          => 'Project:Politiche_pe_privacy',

'versionrequired' => 'E covente la version $1 di MediaWiki',

'ok'                      => 'Va ben',
'retrievedfrom'           => 'Cjapât fûr di $1',
'youhavenewmessages'      => 'Tu âs $1 ($2).',
'newmessageslink'         => 'gnûfs messaçs',
'newmessagesdifflink'     => 'difarencis cu la penultime revision',
'youhavenewmessagesmulti' => 'Tu âs gnûfs messaçs su $1',
'editsection'             => 'cambie',
'editold'                 => 'cambie',
'editsectionhint'         => 'cambie la sezion $1',
'toc'                     => 'Tabele dai contignûts',
'showtoc'                 => 'mostre',
'hidetoc'                 => 'plate',
'thisisdeleted'           => 'Vuelistu cjalâ o ripristinâ $1?',
'viewdeleted'             => 'Vuelistu viodi $1?',
'restorelink'             => '{{PLURAL:$1|une modifiche eliminade|$1 modifichis eliminadis}}',
'feedlinks'               => 'Canâl (feed):',
'site-rss-feed'           => 'Canâl RSS di $1',
'site-atom-feed'          => 'Canâl Atom di $1',
'page-rss-feed'           => 'Canâl RSS par "$1"',
'page-atom-feed'          => 'Canâl Atom par "$1"',
'red-link-title'          => '$1 (ancjemò di scrivi)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Vôs',
'nstab-user'      => 'Pagjine dal utent',
'nstab-media'     => 'Media',
'nstab-special'   => 'Speciâl',
'nstab-project'   => 'Informazions',
'nstab-image'     => 'Figure',
'nstab-mediawiki' => 'Messaç',
'nstab-template'  => 'Model',
'nstab-help'      => 'Jutori',
'nstab-category'  => 'Categorie',

# Main script and global functions
'nospecialpagetext' => "<big>'''Tu âs cirût une pagjine speciâl no valide.'''</big>

Une liste des pagjinis speciâls validis a si pues cjatâ su [[Special:SpecialPages|{{int:specialpages}}]].",

# General errors
'error'           => 'Erôr',
'databaseerror'   => 'Erôr de base di dâts',
'noconnect'       => 'Nus displâs, ma il sît al à al moment cualchi dificoltât tecniche e nol pues conetisi al servidôr de base di dâts. <br />$1',
'nodb'            => 'No si pues selezionâ la base di dâts $1',
'laggedslavemode' => 'Atenzion: La pagjine podarès no segnalâ inzornaments recents.',
'readonlytext'    => "La base di dâts pal moment e je blocade e no si puedin zontâ vôs e fâ modifichis, probabilmentri pe normâl manutenzion de base di dâts, daspò de cuâl dut al tornarà normâl.

L'aministradôr ch'al à metût il bloc al à scrit cheste motivazion: $1",
'filenotfound'    => 'No si pues cjatâ il file "$1".',
'badtitle'        => 'Titul sbaliât',
'badtitletext'    => 'Il titul de pagjine che tu âs inserît nol è valit, al è vuelit, o al veve un erôr tal colegament tra wiki diviersis o tra versions in altris lenghis.
Al podarès vê dentri caratars che no podin jessi doprâts tai titui.',
'viewsource'      => 'Cjale risultive',
'viewsourcefor'   => 'di $1',
'viewsourcetext'  => 'Tu puedis viodi e copiâ la risultive di cheste pagjine:',

# Login and logout pages
'logouttitle'               => 'Jessude dal utent',
'logouttext'                => '<strong>Tu sâs cumò lât fûr.</strong><br />Tu puedis continuâ a doprâ {{SITENAME}} come anonim, o tu puedis jentrâ cul stes o cuntun altri non utent. Note che cualchi pagjine e pues mostrâti ancjemò come jentrât tal sît fin cuant che no tu netis la cache dal sgarfadôr.',
'welcomecreation'           => '== Mandi e benvignût $1! ==

La tô identitât e je stade creade. No stâ dismenteâti di gambiâ lis preferencis di {{SITENAME}}.',
'loginpagetitle'            => 'Jentrade dal utent',
'yourname'                  => 'Non utent',
'yourpassword'              => 'Peraule clâf',
'yourpasswordagain'         => 'Torne a scrivile',
'remembermypassword'        => 'Visiti di me',
'yourdomainname'            => 'Il to domini',
'loginproblem'              => '<b>Al è stât un erôr te jentrade.</b><br />Torne a provâ!',
'login'                     => 'Jentre',
'nav-login-createaccount'   => 'Regjistriti o jentre',
'loginprompt'               => 'Tu âs di vê abilitâts i cookies par jentrâ in {{SITENAME}}.',
'userlogin'                 => 'Regjistriti o jentre',
'logout'                    => 'Jes',
'userlogout'                => 'Jes',
'notloggedin'               => 'No tu sês jentrât',
'nologin'                   => 'No âstu ancjemò une identitât par jentrâ? $1.',
'nologinlink'               => 'Creile cumò',
'createaccount'             => 'Cree une gnove identitât',
'gotaccount'                => 'Âstu za une identitât? $1.',
'gotaccountlink'            => 'Jentre',
'createaccountmail'         => 'par pueste eletroniche',
'badretype'                 => 'Lis peraulis clâfs inseridis no son compagnis.',
'userexists'                => 'Il non utent inserît al è za doprât. Sielç par plasê un non diferent.',
'youremail'                 => 'Email *',
'username'                  => 'Non utent:',
'uid'                       => 'ID utent:',
'prefs-memberingroups'      => 'Al fâs part {{PLURAL:$1|dal grup|dai grups}}:',
'yourrealname'              => 'Non vêr *',
'yourlanguage'              => 'Lenghe di mostrâ',
'yourvariant'               => 'Varietât',
'yournick'                  => 'Stranon (nick):',
'badsig'                    => 'Firme crude invalide; controle i tags HTML.',
'email'                     => 'Pueste eletroniche',
'prefs-help-realname'       => '* Non vêr (opzionâl): se tu sielzis di inserîlu al vignarà doprât par dâti un ricognossiment dal tô lavôr.',
'loginerror'                => 'Erôr te jentrade',
'prefs-help-email'          => 'La direzion di pueste eletroniche e je opzionâl, ma nus permet di mandâti une gnove peraule clâf se tu ti la sês dismenteade. Cun di plui, permet a chei altris di contatâti vie la tô pagjine utent o di discussion cence scugnî mostrâ a ducj la tô identitât.',
'prefs-help-email-required' => 'E covente une direzion di pueste eletroniche.',
'nocookiesnew'              => "L'identitât utent e je stade creade, ma no tu sês jentrât. {{SITENAME}} al dopre i cookies par visâsi dai utents, e tu tu ju âs disabilitâts. Par plasê abilitiju, dopo jentre cul to gnûf non utent e password.",
'nocookieslogin'            => '{{SITENAME}} e dopre i cookies par visâsi dai utents, e tu tu ju âs disabilitâts. Par plasê abilitiju e torne a provâ.',
'noname'                    => 'No tu âs inserît un non utent valit.',
'loginsuccesstitle'         => 'Jentrât cun sucès',
'loginsuccess'              => 'Cumò tu sês jentrât te {{SITENAME}} sicu "$1".',
'nosuchuser'                => 'Nissun utent regjistrât cul non "$1". Controle il non inserît o [[Special:Userlogin/signup|cree tu une gnove identitât]].',
'nosuchusershort'           => 'Nol esist nissun utent cul non "<nowiki>$1</nowiki>". Controle di no vê sbaliât di scrivi.',
'nouserspecified'           => 'Tu scugnis specificâ un non utent.',
'wrongpassword'             => 'La peraule clâf zontade no je juste. Torne par plasê a provâ.',
'wrongpasswordempty'        => 'La peraule clâf inseride e je vueide. Torne a provâ.',
'passwordtooshort'          => 'La tô peraule clâf no je valide o e je masse curte.
E à di jessi di almancul {{PLURAL:$1|1 caratar|$1 caratars}} e jessi difarente dal to non utent.',
'mailmypassword'            => 'Mande une gnove peraule clâf ae me direzion di pueste eletroniche',
'passwordremindertitle'     => 'Gnove peraule clâf temporanie par {{SITENAME}}',
'passwordremindertext'      => 'Cualchidun (probabilmentri tu, de direzion IP $1) al à domandât une gnove peraule clâf par jentrâ in {{SITENAME}} ($4).
Une peraule clâf temporanee par l\'utent "$2" e je stade creade e impuestade a "$3".
Se cheste e jere la tô intenzion tu varâs di jentrâ e sielzi une gnove peraule clâf cumò.

Se no tu âs domandât tu chest o se tu âs cjatât la peraule clâf e no tu vuelis plui cambiâle, tu puedis ignorâ chest messaç e continuâ a doprâ la vecje peraule clâf.',
'noemail'                   => 'Nissune direzion email regjistrade par l\'utent "$1".',
'passwordsent'              => 'Une gnove peraule clâf e je stade mandade ae direzion di pueste eletroniche regjistrade par l\'utent "$1".
Par plasê torne a fâ la jentrade pene che tu la âs ricevude.',
'eauthentsent'              => 'Un messaç di pueste eletroniche di conferme al è stât mandât ae direzion specificade.
Prime di ricevi cualsisei altri messaç di pueste, tu scugnis seguî lis istruzions scritis dal messaç, par confermâ che la identitât e je propi la tô.',
'emailauthenticated'        => 'La tô direzion email e je stade autenticade su $1.',
'emailnotauthenticated'     => 'La tô direzion email no je ancjemò autenticade. No vignaran mandâts messaçs pes funzions ca sot.',
'noemailprefs'              => '<strong>Specifiche une direzion email par fâ lâ cheste funzion.</strong>',
'emailconfirmlink'          => 'Conferme la tô direzion email',
'invalidemailaddress'       => 'La direzion email no pues jessi acetade parcè che no samee intun formât valit. Inserìs par plasê une direzion ben formatade o disvuede chest cjamp.',
'accountcreated'            => 'Identitât creade',
'accountcreatedtext'        => 'La identitât utent par $1 e je stade creade.',
'createaccount-title'       => 'Creazion di une identitât par {{SITENAME}}',
'loginlanguagelabel'        => 'Lenghe: $1',

# Edit page toolbar
'bold_sample'     => 'Test in gruessut',
'bold_tip'        => 'Test in gruessut',
'italic_sample'   => 'Test in corsîf',
'italic_tip'      => 'Test in corsîf',
'link_sample'     => 'Titul dal leam',
'link_tip'        => 'Leams internis',
'extlink_sample'  => 'http://www.example.com titul leam',
'extlink_tip'     => 'Leam esterni (visiti dal prefìs http://)',
'headline_sample' => 'Test dal titul',
'headline_tip'    => 'Titul di nivel 2',
'math_sample'     => 'Inserìs la formule culì',
'math_tip'        => 'Formule matematiche (LaTeX)',
'nowiki_sample'   => 'Inserìs test no formatât culì',
'nowiki_tip'      => 'Ignore la formatazion wiki',
'image_sample'    => 'Esempli.jpg',
'image_tip'       => 'Figure includude',
'media_sample'    => 'Esempli.mp3',
'media_tip'       => 'Leam a un file multimediâl',
'sig_tip'         => 'La tô firme cun ore e date',
'hr_tip'          => 'Rie orizontâl (no stâ doprâle masse spes)',

# Edit pages
'summary'                   => 'Somari',
'subject'                   => 'Argoment (intestazion)',
'minoredit'                 => 'Cheste al è un piçul cambiament',
'watchthis'                 => 'Ten di voli cheste pagjine',
'savearticle'               => 'Salve la pagjine',
'preview'                   => 'Anteprime',
'showpreview'               => 'Mostre anteprime',
'showlivepreview'           => "Anteprime ''live''",
'showdiff'                  => 'Mostre cambiaments',
'anoneditwarning'           => 'No tu sês jentrât cuntun non utent. La to direzion IP e vignarà regjistrade tal storic di cheste pagjine.',
'missingcommenttext'        => 'Inserìs un coment ca sot.',
'summary-preview'           => 'Anteprime dal somari',
'subject-preview'           => 'Anteprime ogjet/intestazion',
'blockedtitle'              => 'Utent blocât',
'blockedtext'               => "<big>'''Chest non utent o direzion IP a son stâts blocâts.'''</big>

Il bloc al è stât metût di $1. La reson furnide e je: ''$2''

* Inizi dal bloc: $8
* Scjadencje dal blocco: $6
* Interval di bloc: $7

Se tu vuelis tu puedis contatâ $1 o un altri [[{{MediaWiki:Grouppage-sysop}}|aministradôr]] par fevelâ dal bloc.

Visiti che no tu puedis doprâ la funzion 'Messaç di pueste a chest utent' se no tu âs specificât une direzion di pueste eletroniche valide tes [[Special:Preferences|preferencis]] e se no tu sês stât blocât al ûs di cheste funzion.

Par plasê inclût la to direzion IP atuâl ($3) o il numar dal bloc (ID #$5) in ogni richieste di sclariments.",
'blockedoriginalsource'     => "Ca sot tu puedis viodi la risultive de pagjine '''$1''':",
'whitelistedittext'         => 'Tu scugnis $1 par cambiâ lis pagjinis.',
'confirmedittitle'          => 'E covente la conferme de direzion di pueste eletroniche pe modifiche de pagjine',
'loginreqtitle'             => 'Si scugne jentrâ',
'loginreqlink'              => 'jentrâ',
'loginreqpagetext'          => 'Tu scugnis $1 par viodi lis altris pagjinis.',
'accmailtitle'              => 'Password mandade.',
'accmailtext'               => 'La password par "$1" e je stade mandade a $2.',
'newarticle'                => '(Gnûf)',
'newarticletext'            => "Tu âs seguît un leam a une pagjine che no esist ancjemò. Par creâ une pagjine, scomence a scrivi tal spazi ca sot (cjale il [[{{MediaWiki:Helppage}}|jutori]] par altris informazions). Se tu sês ca par erôr, frache semplicementri il boton '''Indaûr''' dal to sgarfadôr.",
'noarticletext'             => 'Par cumò nol è nuie in cheste pagjine, tu puedis [[Special:Search/{{PAGENAME}}|cirî chest titul]] in altris pagjinis o [{{fullurl:{{FULLPAGENAME}}|action=edit}} cambiâ cheste pagjine].',
'userpage-userdoesnotexist' => 'La identitât "$1" no je di un utent regjistrât. Controle che tu vuelis pardabon creâ o modificâ cheste pagjine.',
'updated'                   => '(Inzornât)',
'previewnote'               => '<strong>Visiti che cheste e je dome une anteprime, e no je stade ancjemò salvade!</strong>',
'editing'                   => 'Cambiament di $1',
'editingsection'            => 'Cambiament di $1 (sezion)',
'editingcomment'            => 'Cambiament di $1 (coment)',
'editconflict'              => 'Conflit inte modifiche: $1',
'explainconflict'           => 'Cualchidun altri al à cambiât cheste pagjine di cuant che tu âs començât a modificâle.
La aree di test disore e conten il test de pagjine che esist cumò, i tiei cambiaments a son mostrâts inte aree disot.
Tu varâs di inserî di gnûf i tiei cambiaments tal test esistint.
<b>Dome</b> il test in alt al vignarà salvât cuant che tu frachis su "Salve pagjine".<br />',
'editingold'                => '<strong>ATENZION: tu stâs cambiant une version vecje e no inzornade di cheste pagjine. Se tu la salvis, ducj i cambiaments fats di chê volte in ca a laran pierdûts.</strong>',
'yourdiff'                  => 'Difarencis',
'copyrightwarning'          => 'Note: ducj i contribûts a {{SITENAME}} a si considerin come dâts fûr sot de licence $2 (cjale $1 pai detais). Se no tu vuelis che i tiei tescj a podedin jessi modificâts e tornâts a dâ fûr di ognidun cence limits, no stâ mandâju a {{SITENAME}}.<br />
Cun di plui, inviant il test tu declaris che tu âs scrit tu chest o tu lu âs copiât di une sorzint tal domini public o di une sorzint libare.
<strong>NO STÂ MANDÂ MATERIÂL CUVIERT DAL DIRIT DI AUTÔR CENCE AUTORIZAZION!</strong>',
'longpagewarning'           => '<strong>ATENZION: cheste pagjine e je grande $1 kilobytes; cualchi sgarfadôr al podarès vê problemis a modificâ pagjinis di 32kb o plui grandis. Considere par plasê la pussibilitât di dividi la pagjine in sezions plui piçulis.</strong>',
'templatesused'             => 'Modei doprâts par cheste pagjine:',
'templatesusedpreview'      => 'Modei doprâts in cheste anteprime:',
'templatesusedsection'      => 'Modei doprâts in cheste sezion:',
'template-protected'        => '(protezût)',
'template-semiprotected'    => '(semi-protezût)',
'nocreatetext'              => '{{SITENAME}} al à limitât la pussibilitât di creâ gnovis pagjinis ai utents regjistrâts. Tu puedis tornâ indaûr e cambiâ une pagjine che e esist o se no [[Special:UserLogin|jentrâ o creâ une gnove identitât]].',
'nocreate-loggedin'         => 'No tu âs i permès che a coventin par creâ gnovis pagjinis su {{SITENAME}}.',
'recreate-deleted-warn'     => "'''Atenzion: tu stâs par tornâ a creâ une pagjine che e je stade eliminade timp fa.'''

Siguriti che sedi pardabon oportun lâ indevant cun la modifiche di cheste pagjine.
Ve ca par comoditât l'elenc des eliminazions precedentis par cheste pagjine:",

# History pages
'viewpagelogs'        => 'Cjale i regjistris relatîfs a cheste pagjine.',
'nohistory'           => 'Nol è presint un storic dai cambiaments par cheste pagjine.',
'currentrev'          => 'Version atuâl',
'revisionasof'        => 'Version dai $1',
'revision-info'       => 'Version dal $1, autôr: $2',
'previousrevision'    => '← Version plui vecje',
'nextrevision'        => 'Version plui gnove →',
'currentrevisionlink' => 'Version atuâl',
'cur'                 => 'cor',
'next'                => 'prossim',
'last'                => 'ultime',
'page_first'          => 'prime',
'page_last'           => 'ultime',
'histlegend'          => "Confront tra lis versions: sielç lis caselis des versions che ti interessin e frache Invio o il boton in bas.

Leiende: (cur) = difarencis cun la version atuâl, (prec) = difarencis cun la version precedente, '''p''' = piçul cambiament",
'deletedrev'          => '[eliminade]',
'histfirst'           => 'Prime',
'histlast'            => 'Ultime',
'historysize'         => '({{PLURAL:$1|1 byte|$1 bytes}})',
'historyempty'        => '(vueide)',

# Revision feed
'history-feed-item-nocomment' => '$1 ai $2', # user at time

# Revision deletion
'rev-delundel' => 'mostre/plate',

# Diffs
'history-title'           => 'Storic dai cambiaments di "$1"',
'difference'              => '(Difarence jenfri des revisions)',
'lineno'                  => 'Rie $1:',
'compareselectedversions' => 'Confronte versions selezionadis',
'editundo'                => 'anule',
'diff-multi'              => '({{PLURAL:$1|Une version intermedie no mostrade|$1 versions intermediis no mostradis}}.)',

# Search results
'searchresults'         => 'Risultâts de ricercje',
'searchresulttext'      => 'Par plui informazions su lis ricercjis in {{SITENAME}}, cjale [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'        => 'Pal test "[[:$1]]"',
'searchsubtitleinvalid' => 'Pal test "$1"',
'noexactmatch'          => "'''No esist une pagjine cul titul \"\$1\".''' Tu podaressis [[:\$1|creâle tu]].",
'noexactmatch-nocreate' => "'''La pagjine cun titul \"\$1\" no esist.'''",
'titlematches'          => 'Corispondencis tai titui des pagjinis',
'notitlematches'        => 'Nissune corispondence tai titui des pagjinis',
'textmatches'           => 'Corispondencis tal test des pagjinis',
'notextmatches'         => 'Nissune corispondence tal test des pagjinis',
'prevn'                 => 'precedents $1',
'nextn'                 => 'prossims $1',
'viewprevnext'          => 'Cjale ($1) ($2) ($3).',
'search-result-size'    => '$1 ({{PLURAL:$2|une peraule|$2 peraulis}})',
'search-suggest'        => 'Forsit tu cirivis: $1',
'search-interwiki-more' => '(altri)',
'mwsuggest-disable'     => 'Disative i sugjeriments AJAX',
'showingresults'        => "Ca sot {{PLURAL:$1|al è fin a '''1''' risultât|a son fin a '''$1''' risultâts}} scomençant dal numar '''$2'''.",
'showingresultsnum'     => "Ca sot {{PLURAL:$3|al è '''1''' risultât|a son '''$3''' risultâts}} scomençant dal numar '''$2'''.",
'powersearch'           => 'Cîr',
'powersearch-legend'    => 'Ricercje avanzade',
'powersearch-ns'        => 'Cîr tai spazis dai nons:',
'powersearch-redir'     => 'Elenc re-indreçaments',
'search-external'       => 'Ricercje esterne',
'searchdisabled'        => 'La ricercje in {{SITENAME}} no je ative. Tu puedis doprâ Google intant. Sta atent che lis lôr tabelis sul contignût di {{SITENAME}} a puedin jessi pôc inzornadis.',

# Preferences page
'preferences'              => 'Preferencis',
'mypreferences'            => 'mês preferencis',
'prefs-edits'              => 'Numar di cambiaments fats:',
'prefsnologin'             => 'No tu sês jentrât',
'qbsettings'               => 'Sbare svelte',
'qbsettings-none'          => 'Nissune',
'qbsettings-fixedleft'     => 'Fis a Çampe',
'qbsettings-fixedright'    => 'Fis a Drete',
'qbsettings-floatingleft'  => 'Flutuant a çampe',
'qbsettings-floatingright' => 'Flutuant a diestre',
'changepassword'           => 'Gambie peraule clâf',
'skin'                     => 'Mascare',
'math'                     => 'Matematiche',
'dateformat'               => 'Formât de date',
'datedefault'              => 'Nissune preference',
'datetime'                 => 'Date e ore',
'prefs-personal'           => 'Dâts utents',
'prefs-rc'                 => 'Ultins cambiaments & stubs',
'prefs-watchlist'          => 'Tignudis di voli',
'prefs-watchlist-days'     => 'Numar di zornadis di mostrâ inte liste des pagjinis tignudis di voli:',
'prefs-watchlist-edits'    => 'Numar di modifichis di mostrâ inte liste slargjade:',
'prefs-misc'               => 'Variis',
'saveprefs'                => 'Salve lis preferencis',
'resetprefs'               => 'Predeterminât',
'oldpassword'              => 'Vecje peraule clâf',
'newpassword'              => 'Gnove peraule clâf',
'retypenew'                => 'Torne a scrivi chê gnove',
'textboxsize'              => 'Cambiament',
'rows'                     => 'Riis',
'columns'                  => 'Colonis:',
'searchresultshead'        => 'Ricercje',
'resultsperpage'           => 'Risultâts par pagjine',
'contextlines'             => 'Riis par risultât',
'recentchangesdays'        => 'Numar di zornadis di mostrâ tai ultins cambiaments:',
'recentchangescount'       => 'Numar di titui tai ultins cambiaments',
'savedprefs'               => 'Lis preferencis a son stadis salvadis',
'timezonelegend'           => 'Fûs orari',
'timezonetext'             => 'Il numar di oris di diference rispiet ae ore dal servidôr (UTC).',
'localtime'                => 'Ore locâl',
'servertime'               => 'Ore servidôr',
'guesstimezone'            => 'Cjape impostazions dal sgarfadôr',
'allowemail'               => 'Ative la ricezion di messaçs email di bande di altris utents¹',
'prefs-searchoptions'      => 'Opzions de ricercje',
'prefs-namespaces'         => 'Spazis dai nons',
'default'                  => 'predeterminât',
'files'                    => 'Files',

# User rights
'editinguser'             => "Cambiament dai dirits par l'utent '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-groupsmember' => 'Al fâs part di:',

# Groups
'group'               => 'Grup:',
'group-user'          => 'Utents regjistrâts',
'group-autoconfirmed' => 'Utents auto confermâts',
'group-all'           => 'Utents',

'group-user-member' => 'Utent',

'grouppage-sysop' => '{{ns:project}}:Aministradôrs',

# User rights log
'rightslog' => 'Regjistri dai dirits dai utents',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|cambiament|cambiaments}}',
'recentchanges'                     => 'Ultins cambiaments',
'recentchangestext'                 => 'Cheste pagjine e mostre i plui recents cambiaments inte {{SITENAME}}.',
'recentchanges-feed-description'    => 'Chest canâl al ripuarte i cambiaments plui recents ai contignûts di cheste wiki.',
'rcnote'                            => "Ca sot tu cjatis {{PLURAL:$1|l'ultin cambiament|i ultins '''$1''' cambiaments}} al sît {{PLURAL:$2|intes ultimis 24 oris|tes ultimis '''$2''' zornadis}}; i dâts a son inzornâts ai $4 a lis $5.",
'rcnotefrom'                        => "Ca sot i cambiaments dal '''$2''' (fintremai al '''$1''').",
'rclistfrom'                        => 'Mostre i ultins cambiaments dal $1',
'rcshowhideminor'                   => '$1 i piçui cambiaments',
'rcshowhidebots'                    => '$1 i bots',
'rcshowhideliu'                     => '$1 utents jentrâts',
'rcshowhideanons'                   => '$1 utents anonims',
'rcshowhidepatr'                    => '$1 cambiaments verificâts',
'rcshowhidemine'                    => '$1 miei cambiaments',
'rclinks'                           => 'Mostre i ultins $1 cambiaments tes ultimis $2 zornadis<br />$3',
'diff'                              => 'difarencis',
'hist'                              => 'stor',
'hide'                              => 'plate',
'show'                              => 'mostre',
'minoreditletter'                   => 'p',
'newpageletter'                     => 'G',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[tignude di voli di {{PLURAL:$1|un utent|$1 utents}}]',
'rc_categories'                     => 'Limite aes categoriis (dividilis cun "|")',
'rc_categories_any'                 => 'Cualsisei',
'newsectionsummary'                 => '/* $1 */ gnove sezion',

# Recent changes linked
'recentchangeslinked'          => 'Cambiaments leâts',
'recentchangeslinked-title'    => 'Cambiaments leâts a "$1"',
'recentchangeslinked-noresult' => 'Nissun cambiament aes pagjinis leadis tal periodi specificât.',
'recentchangeslinked-summary'  => "Cheste pagjine speciâl e mostre i cambiaments plui recents aes pagjinis leadis a chê specificade (o leadis ai elements intune categorie specificade). Lis [[Special:Watchlist|pagjinis tignudis di voli]] a son mostradis in '''gruessut'''.",
'recentchangeslinked-page'     => 'Non de pagjine:',
'recentchangeslinked-to'       => 'Mostre dome i cambiaments aes pagjinis leadis a chê specificade',

# Upload
'upload'             => 'Cjame sù un file',
'uploadbtn'          => 'Cjame sù un file',
'reupload'           => 'Torne a cjamâ sù',
'uploadnologin'      => 'No jentrât',
'uploadnologintext'  => 'Tu scugnis [[Special:UserLogin|jentrâ]] cul to non utent par cjamâ sù files.',
'uploaderror'        => 'Erôr cjamant sù',
'uploadtext'         => "Dopre la form ca sot par cjamâ sù un file, par cjalâ o cirî i files cjamâts sù in precedence va te [[Special:ImageList|liste dai files cjamâts sù]], lis cjamadis e lis eliminazions a son ancje regjistrâts tal [[Special:Log/upload|regjistri des cjamadis]].

Par includi une figure intune pagjine, dopre un leam inte form
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:file.jpg]]</nowiki>''',
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:file.png|alt text]]</nowiki>''' or
'''<nowiki>[[</nowiki>{{ns:media}}<nowiki>:file.ogg]]</nowiki>''' par un leam diret al file.",
'uploadlog'          => 'regjistri cjamâts sù',
'uploadlogpage'      => 'Regjistri dai files cjamâts sù',
'uploadlogpagetext'  => 'Ca sot e je une liste dai file cjamâts su di recent.',
'filename'           => 'Non dal file',
'filedesc'           => 'Descrizion',
'fileuploadsummary'  => 'Somari:',
'filestatus'         => 'Stât dal copyright:',
'filesource'         => 'Surzint:',
'uploadedfiles'      => 'Files cjamâts sù',
'ignorewarning'      => 'Ignore avîs e salve instès il file.',
'ignorewarnings'     => 'Ignore i avîs',
'badfilename'        => 'File non gambiât in "$1".',
'successfulupload'   => 'Cjamât sù cun sucès',
'savefile'           => 'Salve file',
'uploadedimage'      => 'cjamât sù "$1"',
'uploaddisabled'     => 'Nus displâs, par cumò no si pues cjamâ sù robe.',
'uploaddisabledtext' => 'Lis cjamadis a son disativâts su cheste wiki.',
'sourcefilename'     => 'Non dal file origjinâl:',
'destfilename'       => 'Non dal file di destinazion:',

# Special:ImageList
'imagelist'             => 'Liste des figuris',
'imagelist_date'        => 'Date',
'imagelist_name'        => 'Non',
'imagelist_user'        => 'Utent',
'imagelist_size'        => 'Dimension in bytes',
'imagelist_description' => 'Descrizion',

# Image description page
'filehist'                  => 'Storic dal file',
'filehist-help'             => 'Frache suntune date/ore par viodi il file cemût che al jere in chel moment.',
'filehist-current'          => 'corint',
'filehist-datetime'         => 'Date/Ore',
'filehist-user'             => 'Utent',
'filehist-dimensions'       => 'Dimensions',
'filehist-filesize'         => 'Dimension dal file',
'filehist-comment'          => 'Coment',
'imagelinks'                => 'Leams de figure',
'linkstoimage'              => '{{PLURAL:$1|La pagjine ca sot e je leade|Lis $1 pagjinis ca sot a son leadis}} a cheste figure:',
'nolinkstoimage'            => 'No son pagjinis leadis a chest file.',
'sharedupload'              => 'Chest file al è condivîs e al pues jessi doprât di altris progjets.',
'shareduploadwiki'          => 'Cjale par plasê la [pagjine di descrizion dal file $1] par altris informazions.',
'shareduploadwiki-desc'     => 'La descrizion su la $1 intal dipuesit condividût e ven mostrade ca sot.',
'shareduploadwiki-linktext' => 'pagjine di descrizion dal file',
'noimage'                   => 'Nol esist un file cun chest non, ma tu puedis $1 tu.',
'noimage-linktext'          => 'cjamâlu sù',
'uploadnewversion-linktext' => 'Cjame sù une gnove version di chest file',

# File deletion
'filedelete'        => 'Elimine $1',
'filedelete-legend' => 'Elimine il file',
'filedelete-submit' => 'Elimine',

# MIME search
'mimesearch' => 'Ricercje MIME',
'mimetype'   => 'Gjenar MIME:',
'download'   => 'discjame',

# List redirects
'listredirects' => 'Liste des redirezions',

# Unused templates
'unusedtemplates' => 'Modei no doprâts',

# Random page
'randompage' => 'Une pagjine a câs',

# Random redirect
'randomredirect' => 'Un re-indreçament casuâl',

# Statistics
'statistics'             => 'Statistichis',
'sitestats'              => 'Statistichis dal sît',
'userstats'              => 'Statistichis dai utents',
'sitestatstext'          => "Tu puedis cjatâ in dut '''\$1''' {{PLURAL:\$1|pagjine|pagjinis}} inte base di dâts. Chest numar al inclût pagjinis di \"discussion\", pagjinis su la {{SITENAME}}, pagjinis cun pocjis peraulis, re-indreçaments, e altris che probabilmentri no si puedin considerâ pardabon come pagjinis di contignût.
Gjavant chestis, o vin '''\$2''' {{PLURAL:\$2|pagjine che e je|pagjinis che a son}} probabilmentri di contignût legjitim.

'''\$8''' {{PLURAL:\$8|file al è stât cjamât|files a son stâts cjamâts}} sù.

O vin vût in dut '''\$3''' {{PLURAL:\$3|viodude|viodudis}} des pagjinis e '''\$4''' {{PLURAL:\$4|cambiament|cambiaments}} aes pagjinis di cuant che la wiki e je stade implantade. Chest al vûl dî une medie di '''\$5''' cambiaments par pagjine, e '''\$6''' viodudis par ogni cambiament.

La code dai [http://www.mediawiki.org/wiki/Manual:Job_queue procès di fâ] e conten {{PLURAL:\$7|'''1''' element|'''\$7''' elements}}.",
'userstatstext'          => "{{PLURAL:$1|Al è '''1''' [[Special:ListUsers|utent]] regjistrât|A son '''$1''' [[Special:ListUsers|utents]] regjistrâts}}, di chescj  '''$2''' (o il '''$4%''') {{PLURAL:$2|al è|a son}} $5 .",
'statistics-mostpopular' => 'Pagjinis plui visitadis',

'disambiguations' => 'Pagjinis di disambiguazion',

'doubleredirects' => 'Re-indreçaments doplis',

'brokenredirects'        => 'Re-indreçaments che no funzionin',
'brokenredirectstext'    => 'I re-indreçaments ca sot a mandin a pagjinis che no esistin:',
'brokenredirects-edit'   => '(cambie)',
'brokenredirects-delete' => '(elimine)',

'withoutinterwiki'        => 'Pagjinis cence leams interwiki',
'withoutinterwiki-submit' => 'Mostre',

'fewestrevisions' => 'Vôs con mancul revisions',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|byte|bytes}}',
'ncategories'             => '$1 {{PLURAL:$1|categorie|categoriis}}',
'nlinks'                  => '$1 {{PLURAL:$1|leam|leams}}',
'nmembers'                => '$1 {{PLURAL:$1|element|elements}}',
'nviews'                  => '$1 {{PLURAL:$1|viodude|viodudis}}',
'lonelypages'             => 'Pagjinis solitaris',
'lonelypagestext'         => 'Nissune pagjine in {{SITENAME}} e à leams aes pagjinis ca sot.',
'uncategorizedpages'      => 'Pagjinis cence categorie',
'uncategorizedcategories' => 'Categoriis cence categorie',
'uncategorizedimages'     => 'Files cence une categorie',
'uncategorizedtemplates'  => 'Modei cence une categorie',
'unusedcategories'        => 'Categoriis no dopradis',
'unusedimages'            => 'Files no doprâts',
'popularpages'            => 'Pagjinis popolârs',
'wantedcategories'        => 'Categoriis desideradis',
'wantedpages'             => 'Pagjinis desideradis',
'mostlinked'              => 'Pagjinis a cui pontin il maiôr numar di leams',
'mostlinkedcategories'    => 'Categoriis a cui pontin il maiôr numar di leams',
'mostlinkedtemplates'     => 'Modei plui doprâts',
'mostcategories'          => 'Vôs cul maiôr numar di categoriis',
'mostimages'              => 'Figuris a cui pontin il maiôr numar di leams',
'mostrevisions'           => 'Vôs cul maiôr numar di revisions',
'prefixindex'             => 'Tabele des vôs par letare iniziâl',
'shortpages'              => 'Pagjinis curtis',
'longpages'               => 'Pagjinis lungjis',
'deadendpages'            => 'Pagjinis cence usite',
'protectedpages'          => 'Pagjinis protezudis',
'listusers'               => 'Liste dai utents',
'newpages'                => 'Gnovis pagjinis',
'newpages-username'       => 'Non utent:',
'ancientpages'            => 'Pagjinis plui vecjis',
'move'                    => 'Môf',
'movethispage'            => 'Môf cheste pagjine',

# Book sources
'booksources'    => 'Fonts librariis',
'booksources-go' => 'Va',

# Special:Log
'specialloguserlabel'  => 'Utent:',
'speciallogtitlelabel' => 'Titul:',
'log'                  => 'Regjistris',
'all-logs-page'        => 'Ducj i regjistris',
'log-search-submit'    => 'Va',
'alllogstext'          => 'Viodude combinade di ducj i regjistris disponibii di {{SITENAME}}.
Tu puedis strenzi la viodude sielzint un gjenar di regjistri, un non utent e/o la vôs che ti interesse (ducj e doi i cjamps a son sensibii al maiuscul/minuscul).',
'logempty'             => 'Nissun element corispondint tal regjistri.',

# Special:AllPages
'allpages'          => 'Dutis lis pagjinis',
'alphaindexline'    => 'di $1 a $2',
'nextpage'          => 'Prossime pagjine ($1)',
'prevpage'          => 'Pagjinis precedentis ($1)',
'allpagesfrom'      => 'Mostre pagjinis scomençant di:',
'allarticles'       => 'Dutis lis vôs',
'allinnamespace'    => 'Dutis lis pagjinis (non dal spazi $1)',
'allnotinnamespace' => 'Dutis lis pagjinis (no tal non dal spazi $1)',
'allpagesprev'      => 'Precedent',
'allpagesnext'      => 'Prossim',
'allpagessubmit'    => 'Va',
'allpagesprefix'    => 'Mostre lis pagjinis che a scomencin cun:',

# Special:Categories
'categories'         => 'Categoriis',
'categoriespagetext' => 'Lis categoriis ca sot a àn dentri pagjinis o elements multimediâi.
Lis [[Special:UnusedCategories|categoriis no dopradis]] no son mostradis culì.
Cjale ancje lis [[Special:WantedCategories|categoriis desideradis]].',
'categoriesfrom'     => 'Mostre lis categoriis scomençant di:',

# Special:ListUsers
'listusersfrom'    => 'Mostre i utents scomençant di:',
'listusers-submit' => 'Mostre',

# Special:ListGroupRights
'listgrouprights-group'  => 'Grup',
'listgrouprights-rights' => 'Dirits',

# E-mail user
'emailuser'       => 'Messaç di pueste a chest utent',
'emailpage'       => 'Mande un messaç di pueste eletroniche al utent',
'defemailsubject' => 'Messaç di {{SITENAME}}',
'noemailtitle'    => 'Nissune direzion email',
'noemailtext'     => 'Chest utent nol à specificât une direzion di pueste valide o al à sielzût di no ricevi pueste di altris utents.',
'emailfrom'       => 'Di:',
'emailto'         => 'A:',
'emailsubject'    => 'Ogjet:',
'emailmessage'    => 'Messaç:',
'emailsend'       => 'Mande',
'emailccme'       => 'Mandimi une copie.',

# Watchlist
'watchlist'            => 'Tignûts di voli',
'mywatchlist'          => 'Tignûts di voli',
'watchlistfor'         => "(par '''$1''')",
'nowatchlist'          => 'Nissun element al è tignût di voli.',
'watchnologin'         => 'No tu sês jentrât',
'watchnologintext'     => "Tu 'nd âs di [[Special:UserLogin|jentrâ]] par modificâ la liste des pagjinis tignudis di voli.",
'addedwatch'           => 'Zontât aes pagjinis tignudis di voli',
'addedwatchtext'       => "La pagjine \"<nowiki>\$1</nowiki>\" e je stade zontade ae [[Special:Watchlist|liste di chês tignudis di voli]].
Tal futûr i cambiaments a cheste pagjine e ae pagjine di discussion relative a saran segnalâts ca,
e la pagjine e sarà '''gruessute''' te [[Special:RecentChanges|liste dai ultins cambiaments]] cussì che tu puedis notâle daurman.

<p>Se tu vuelis gjavâle de liste pi indevant, frache su \"No stâ tignî di voli\" te sbare in alt.",
'removedwatch'         => 'Gjavade de liste',
'removedwatchtext'     => 'La pagjine "<nowiki>$1</nowiki>" e je stade gjavade de liste di chês tignudis di voli.',
'watch'                => 'Ten di voli',
'watchthispage'        => 'Ten di voli cheste pagjine',
'unwatch'              => 'No stâ tignî di voli',
'unwatchthispage'      => 'No stâ tignî di voli plui',
'notanarticle'         => 'Cheste pagjine no je une vôs',
'watchnochange'        => 'Nissun element di chei tignûts di voli al è stât cambiât tal periodi mostrât.',
'watchlist-details'    => '{{PLURAL:$1|E je $1 pagjine tignude|A son $1 pagjinis tignudis}} di voli, cence contâ lis pagjinis di discussion.',
'wlheader-enotif'      => '* Notifiche par pueste eletroniche ativade.',
'wlheader-showupdated' => "* Lis pagjinis gambiadis de ultime volte che tu lis âs cjaladis a son mostradis in '''gruessut'''",
'watchlistcontains'    => 'Tu stâs tignint di voli $1 {{PLURAL:$1|pagjine|pagjinis}}.',
'wlnote'               => "Ca sot {{PLURAL:$1|al è il cambiament plui recent|a son i '''$1''' cambiaments plui recents}} {{PLURAL:$2|inte ultime ore|intes '''$2''' oris passadis}}.",
'wlshowlast'           => 'Mostre ultimis $1 oris $2 zornadis $3',
'watchlist-show-bots'  => 'Mostre i cambiaments dai bots',
'watchlist-hide-bots'  => 'Plate i cambiaments dai bots',
'watchlist-show-own'   => 'Mostre i miei cambiaments',
'watchlist-hide-own'   => 'Plate i miei cammbiaments',
'watchlist-show-minor' => 'Mostre i miei piçui cambiaments',
'watchlist-hide-minor' => 'Plate i piçui cambiaments',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Daûr a zontâ aes pagjinis tignudis di voli...',
'unwatching' => 'Daûr a gjavâ des pagjinis tignudis di voli...',

'enotif_impersonal_salutation' => 'Utent di {{SITENAME}}',
'changed'                      => 'cambiade',
'created'                      => 'creade',
'enotif_anon_editor'           => 'utent anonim $1',

# Delete/protect/revert
'deletepage'                  => 'Elimine pagjine',
'confirm'                     => 'Conferme',
'excontent'                   => "il contignût al jere: '$1'",
'excontentauthor'             => "il contignût al jere: '$1' (e al veve contribuît dome '$2')",
'exbeforeblank'               => "il contignût prime di disvuedâ al jere: '$1'",
'exblank'                     => 'pagjine vueide',
'delete-confirm'              => 'Elimine "$1"',
'delete-legend'               => 'Elimine',
'historywarning'              => 'Atenzion: la pagjine che tu stâs eliminant e à un storic.',
'confirmdeletetext'           => 'Tu stâs par eliminâ par simpri une pagjine insieme cun dut il so storic.
Par plasê, da la conferme che tu vuelis fâlu, che tu capissis lis conseguencis e che tu lu stâs fasint tal rispiet de [[{{MediaWiki:Policy-url}}|politiche dal progjet]].',
'actioncomplete'              => 'Azion completade',
'deletedtext'                 => '"<nowiki>$1</nowiki>" al è stât eliminât.
Cjale $2 par une liste des ultimis eliminazions.',
'deletedarticle'              => 'eliminât "$1"',
'dellogpage'                  => 'Regjistri des eliminazions',
'deletionlog'                 => 'regjistri eliminazions',
'reverted'                    => 'Tornât ae version precedente',
'deletecomment'               => 'Reson pe eliminazion',
'deleteotherreason'           => 'Altri motîf o motîf in plui:',
'deletereasonotherlist'       => 'Altri motîf',
'rollbacklink'                => 'revoche',
'protectlogpage'              => 'Regjistri des protezions',
'protectedarticle'            => '$1 protezût',
'protect-title'               => 'Protezint "$1"',
'protect-legend'              => 'Conferme protezion',
'protectcomment'              => 'Reson pe protezion',
'protectexpiry'               => 'Scjadence:',
'protect_expiry_invalid'      => 'Scjadence no valide.',
'protect_expiry_old'          => 'La scjadence e je za passade.',
'protect-unchain'             => 'Sbloche i permès di spostament',
'protect-text'                => 'Ca tu puedis viodi e cambiâ il nivel di protezion pe pagjine <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-access'       => 'No tu âs i permès che a coventis par cambiâ i nivei di protezion de pagjine.
Lis impuestazions atuâls pe pagjine a son <strong>$1</strong>:',
'protect-cascadeon'           => 'Cheste pagjine e je blocade par cumò parcè che e je includude {{PLURAL:$1|inte pagjine|intes pagjinis}} culì sot, dulà che e je ative la protezion ricorsive.
Tu puedis cambiâ il nivel di protezion di cheste pagjine, ma chest nol varà efiets su la protezion ricorsive.',
'protect-default'             => '(predeterminât)',
'protect-fallback'            => 'Al covente il permès "$1"',
'protect-level-autoconfirmed' => 'Dome utents regjistrâts',
'protect-level-sysop'         => 'Dome aministradôrs',
'protect-summary-cascade'     => 'a discjadude',
'protect-expiring'            => 'e scjât: $1 (UTC)',
'protect-cascade'             => 'Protezion ricorsive (estendude a dutis lis pagjinis includudis in cheste).',
'protect-cantedit'            => 'No tu puedis cambiâ i nivei di protezion par cheste pagjine, parcè che no tu âs i permès par modificâle.',
'restriction-type'            => 'Permès:',
'restriction-level'           => 'Nivel di restrizion:',
'pagesize'                    => '(bytes)',

# Restrictions (nouns)
'restriction-edit'   => 'Cambie',
'restriction-move'   => 'Spostament',
'restriction-create' => 'Creazion',

# Undelete
'undeletebtn'            => 'Ripristine',
'undeletecomment'        => 'Coment:',
'undeletedarticle'       => 'al à recuperât "[[$1]]"',
'undelete-search-submit' => 'Cîr',

# Namespace form on various pages
'namespace'      => 'Non dal spazi:',
'invert'         => 'Invertìs selezion',
'blanknamespace' => '(Principâl)',

# Contributions
'contributions' => 'Contribûts dal utent',
'mycontris'     => 'Miei contribûts',
'contribsub2'   => 'Par $1 ($2)',
'nocontribs'    => 'Nissun cambiament che al rispiete chescj criteris cjatât.',
'uctop'         => ' (su)',
'month'         => 'Scomençant dal mês (e prime):',
'year'          => 'Scomençant dal an (e prime):',

'sp-contributions-newbies-sub' => 'Pai gnûfs utents',
'sp-contributions-blocklog'    => 'Regjistri dai blocs',
'sp-contributions-submit'      => 'Cîr',

# What links here
'whatlinkshere'           => 'Leams a cheste vôs',
'whatlinkshere-title'     => 'Pagjinis che a son leadis a "$1"',
'whatlinkshere-page'      => 'Pagjine:',
'linklistsub'             => '(Liste di leams)',
'linkshere'               => "Lis pagjinis ca sot a son leadis a '''[[:$1]]''':",
'nolinkshere'             => "Nissune pagjine e à leams a '''[[:$1]]'''.",
'nolinkshere-ns'          => "No son pagjine leadis a '''[[:$1]]''' intal spazi dai nons sielt.",
'isredirect'              => 'pagjine di reindirizament',
'istemplate'              => 'includude',
'whatlinkshere-prev'      => '{{PLURAL:$1|precedent|precedents $1}}',
'whatlinkshere-next'      => '{{PLURAL:$1|sucessîf|sucessîfs $1}}',
'whatlinkshere-links'     => '← leams',
'whatlinkshere-hidelinks' => '$1 leams',
'whatlinkshere-filters'   => 'Filtris',

# Block/unblock
'blockip'            => 'Bloche utent',
'blockip-legend'     => "Bloche l'utent",
'ipaddress'          => 'Direzion IP:',
'ipadressorusername' => 'Direzion IP o non utent:',
'ipbexpiry'          => 'Scjadence dal bloc:',
'ipbreason'          => 'Reson dal bloc:',
'ipbsubmit'          => 'Bloche chest utent',
'ipboptions'         => '2 oris:2 hours,1 zornade:1 day,3 zornadis:3 days,1 setemane:1 week,2 setemanis:2 weeks,1 mês:1 month,3 mês:3 months,6 mês:6 months,1 an:1 year,infinît:infinite', # display1:time1,display2:time2,...
'ipblocklist'        => 'Utents e direzions IP blocadis',
'blocklink'          => 'bloche',
'unblocklink'        => 'sbloche',
'contribslink'       => 'contribûts',
'blocklogpage'       => 'Regjistri dai blocs',
'blocklogentry'      => 'al à blocât "[[$1]]"; scjadence $2 $3',

# Developer tools
'lockdb'  => 'Bloche base di dâts',
'lockbtn' => 'Bloche base di dâts',

# Move page
'move-page'        => 'Spostament di $1',
'move-page-legend' => 'Môf pagjine',
'movepagetext'     => "Cun il formulari ca sot tu puedis gambiâ il non a une pagjine, movint dut il sô storic al gnûf non.
Il vieri titul al deventarà une pagjine di reindirizament al gnûf titul. I leams ae vecje pagjine no saran gambiâts; verifiche
par plasê che no sedin reindirizaments doplis o no funzionants.
Tu sês responsabil che i leams a continui a mandâ tal puest just.

Note che la pagjine '''no''' sarà movude se e je za une pagjine cul gnûf titul, a mancul che no sedi vueide o un reindirizament e
cence un storic. Chest al vûl dî che tu puedis tornâ a movi la pagjine tal titul precedent, se
tu 'nd âs sbaliât e che no tu puedis sorescrivi une pagjine esistìnte.

<b>ATENZION!</b>
Chest al pues jessi un cambiament drastic e surprendint par une pagjine popolâr;
tu âs di cognossi lis conseguencis prime di lâ indevant.",
'movepagetalktext' => "La pagjine di discussion corispuindinte e vegnarà ancje movude in automaticc, '''fûr che in chescj câs:'''
* Il spostament de pagjine e je tra spazis dai nons diviers
* Sot dal gnûf titul e esist za une pagjine di discussion (e no je vueide)
* Tu âs gjavât la sponte te casele culì sot.

In chescj câs, tu varâs di movi o unî a man lis informazions contignudis te pagjine di discussion, se tu lu desideris.",
'movearticle'      => 'Môf la vôs',
'newtitle'         => 'Al gnûf titul',
'move-watch'       => 'Ten di voli cheste pagjine',
'movepagebtn'      => 'Môf pagjine',
'pagemovedsub'     => 'Movude cun sucès',
'movepage-moved'   => '<big>\'\'\'"$1" e je stade movude al titul "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'    => 'Une pagjine cun chest non e esist za, o il non sielt nol è valit.
Sielç par plasê un altri non.',
'talkexists'       => "'''La pagjine e je stade movude cun sucès, ma no si à podût movi la pagjine di discussion parcè che e esist za tal gnûf titul. Trasferìs il contignût a man par plasê.'''",
'movedto'          => 'Movude in',
'movetalk'         => 'Môf ancje la pagjine di discussion, se pussibil.',
'1movedto2'        => '$1 movût in $2',
'movelogpage'      => 'Regjistri des pagjinis movudis',
'movelogpagetext'  => 'Ca sot e je une liste des pagjinis movudis.',
'movereason'       => 'Reson',
'revertmove'       => 'ripristine',
'delete_and_move'  => 'Elimine e môf',

# Export
'export'        => 'Espuarte pagjinis',
'exportcuronly' => 'Inclût dome la revision corinte, no dut il storic',

# Namespace 8 related
'allmessages'         => 'Ducj i messaçs di sistem',
'allmessagesname'     => 'Non',
'allmessagesdefault'  => 'Test predeterminât',
'allmessagescurrent'  => 'Test curint',
'allmessagestext'     => 'Cheste e je une liste dai messaçs di sisteme disponibii tal non dal spazi MediaWiki:',
'allmessagesmodified' => 'Mostre dome modificâts',

# Thumbnails
'thumbnail-more'  => 'Slargje',
'filemissing'     => 'File mancjant',
'thumbnail_error' => 'Erôr inte creazion de miniature: $1',

# Special:Import
'import'        => 'Impuarte pagjinis',
'importfailed'  => 'Impuartazion falide: $1',
'importnotext'  => 'Vueit o cence test',
'importsuccess' => 'Impuartât cun sucès!',

# Import log
'importlogpage' => 'Regjistris des impuartazions',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'La tô pagjine utent',
'tooltip-pt-anonuserpage'         => 'La pagjine utent di cheste direzion IP',
'tooltip-pt-mytalk'               => 'La mê pagjine di discussion',
'tooltip-pt-anontalk'             => 'Discussions dai cambiaments fats di cheste direzion IP',
'tooltip-pt-preferences'          => 'Lis mês preferencis',
'tooltip-pt-watchlist'            => 'La liste des pagjinis che tu stâs tignint di voli',
'tooltip-pt-mycontris'            => 'Liste dai miei contribûts',
'tooltip-pt-login'                => 'La regjistrazion e je conseade, ancje se no obligatorie.',
'tooltip-pt-anonlogin'            => 'La regjistrazion e je conseade, ma no si scugne fâle',
'tooltip-pt-logout'               => 'Jes (logout)',
'tooltip-ca-talk'                 => 'Discussions su cheste pagjine',
'tooltip-ca-edit'                 => 'Tu puedis cambiâ cheste pagjine. Par plasê dopre il boton de anteprime prime di salvâ.',
'tooltip-ca-addsection'           => 'Zonte un coment a cheste discussion',
'tooltip-ca-viewsource'           => 'Cheste pagjine e je protezude, ma tu puedis viodi la sô risultive.',
'tooltip-ca-history'              => 'Versions precedentis di cheste pagjine.',
'tooltip-ca-protect'              => 'Protêç cheste pagjine',
'tooltip-ca-delete'               => 'Elimine cheste pagjine',
'tooltip-ca-move'                 => 'Môf cheste pagjine (cambie il titul)',
'tooltip-ca-watch'                => 'Zonte cheste pagjine ae liste des pagjinis tignudis di voli',
'tooltip-ca-unwatch'              => 'Gjave cheste pagjine de liste des pagjinis tignudis di voli',
'tooltip-search'                  => 'Cîr in cheste wiki',
'tooltip-search-go'               => 'Va a une pagjine cul titul esat inserît, se e esist',
'tooltip-search-fulltext'         => 'Cîr il test inserît intes pagjinis',
'tooltip-p-logo'                  => 'Pagjine principâl',
'tooltip-n-mainpage'              => 'Visite la pagjine principâl',
'tooltip-n-portal'                => 'Descrizion dal progjet, ce che tu puedis fâ e dulà che tu puedis cjatâ lis robis',
'tooltip-n-currentevents'         => 'Informazions sui events di atualitât',
'tooltip-n-recentchanges'         => 'Liste dai ultins cambiaments inte wiki.',
'tooltip-n-randompage'            => 'Mostre une pagjine casuâl',
'tooltip-n-help'                  => 'Pagjinis di aiût',
'tooltip-t-whatlinkshere'         => 'Liste di dutis lis pagjinis che a son leadis a cheste',
'tooltip-t-recentchangeslinked'   => 'Liste dai ultins cambiaments intes pagjinis leadis a cheste',
'tooltip-feed-rss'                => 'Cjanâl RSS par cheste pagjine',
'tooltip-feed-atom'               => 'Cjanâl Atom par cheste pagjine',
'tooltip-t-contributions'         => 'Liste dai contribûts di chest utent',
'tooltip-t-emailuser'             => 'Mande un messaç di pueste eletroniche a chest utent',
'tooltip-t-upload'                => 'Cjame sù files multimediâi',
'tooltip-t-specialpages'          => 'Liste di dutis lis pagjinis speciâls',
'tooltip-t-print'                 => 'Version apueste pe stampe di cheste pagjine',
'tooltip-t-permalink'             => 'Leam permanent a cheste version de pagjine',
'tooltip-ca-nstab-main'           => 'Cjale la vôs',
'tooltip-ca-nstab-user'           => 'Cjale la pagjine dal utent',
'tooltip-ca-nstab-special'        => 'Cheste e je une pagjine speciâl e no pues jessi cambiade',
'tooltip-ca-nstab-project'        => 'Cjale la pagjine dal progjet',
'tooltip-ca-nstab-image'          => 'Cjale la pagjine dal file',
'tooltip-ca-nstab-mediawiki'      => 'Cjale il messaç di sisteme',
'tooltip-ca-nstab-template'       => 'Cjale il model',
'tooltip-ca-nstab-help'           => 'Cjale la pagjine dal jutori',
'tooltip-ca-nstab-category'       => 'Cjale la pagjine de categorie',
'tooltip-minoredit'               => 'Segne cheste come une piçul cambiament',
'tooltip-save'                    => 'Salve i tiei cambiaments',
'tooltip-preview'                 => 'Anteprime dai tiei cambiaments, doprile par plasê prime di salvâ!',
'tooltip-diff'                    => 'Mostre i cambiaments che tu âs fat al test.',
'tooltip-compareselectedversions' => 'Viôt lis difarencis framieç lis dôs versions di cheste pagjine selezionadis.',
'tooltip-watch'                   => 'Zonte cheste pagjine ae liste di chês tignudis di voli',

# Stylesheets
'monobook.css' => '/* modifiche chest file par personalizâ la skin monobook par dut il sît */',

# Attribution
'anonymous'        => 'Utent(s) anonim(s) di {{SITENAME}}',
'siteuser'         => 'Utent $1 di {{SITENAME}}',
'lastmodifiedatby' => 'Cheste pagjine e je stade cambiade pe ultime volte a lis $2, $1 di $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Basât sul lavôr di $1.',
'others'           => 'altris',
'siteusers'        => 'Utents  $1 di {{SITENAME}}',
'creditspage'      => 'Pagjine dai ricognossiments',
'nocredits'        => 'Nissune informazion sui ricognossiments disponibil par cheste pagjine.',

# Info page
'infosubtitle'   => 'Informazions pe pagjine',
'numedits'       => 'Numar di cambiaments (vôs): $1',
'numtalkedits'   => 'Numar di cambiaments (pagjine di discussion): $1',
'numwatchers'    => 'Numar di chei che e àn cjalât: $1',
'numauthors'     => 'Numar di autôrs diviers (vôs): $1',
'numtalkauthors' => 'Numar di autôrs diviers (pagjine di discussion): $1',

# Math options
'mw_math_png'    => 'Torne simpri PNG',
'mw_math_simple' => 'HTML se une vore sempliç, se no PNG',
'mw_math_html'   => 'HTML se pussibil se no PNG',
'mw_math_source' => 'Lassile come TeX (par sgarfadôrs testuâi)',
'mw_math_modern' => 'Racomandât pai sgarfadôrs testuâi',
'mw_math_mathml' => 'MathML se pussibil (sperimentâl)',

# Browsing diffs
'previousdiff' => '← Difarence precedente',
'nextdiff'     => 'Prossime difarence →',

# Media information
'thumbsize'            => 'Dimension miniature:',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|pagjine|pagjinis}}',
'file-info'            => 'Dimensions: $1, gjenar MIME: $2',
'file-info-size'       => '($1 × $2 pixel, dimensions: $3, gjenar MIME: $4)',
'file-nohires'         => '<small>No son disponibilis versions cun risoluzion plui alte.</small>',
'svg-long-desc'        => '(file tal formât SVG, dimensions nominâls $1 × $2 pixels, dimensions dal file: $3)',
'show-big-image'       => 'Version a risoluzion plene',
'show-big-image-thumb' => '<small>Dimensions di cheste anteprime: $1 × $2 pixels</small>',

# Special:NewImages
'newimages'     => 'Galarie dai gnûfs files',
'imagelisttext' => 'Ca sot e je une liste di $1 {{PLURAL:$1|file|files}} ordenâts $2.',
'showhidebots'  => '($1 i bots)',
'noimages'      => 'Nuie di viodi.',
'ilsubmit'      => 'Cîr',
'bydate'        => 'par date',

# Bad image list
'bad_image_list' => 'Il formât al è cussi:

a vegnin considerâts dome i elements des listis (riis che a scomencin cul catatar *). 
Il prin leam intune rie al à di jessi un leam aun file indesiderâtI.
I leams sucessîfs, su la stesse rie, a son considerâts come ecezions (ven a stâi pagjinis dulà che il file al pues jessi inserît normalmentri).',

# Metadata
'metadata'          => 'Metadâts',
'metadata-help'     => 'Chest file al conten informazions in plui, probabilmentri zontadis de fotocjamare o dal scanner doprât par creâlu o digjitalizâlu.
Se il file al è stât cambiât rispiet al so stât origjinâl, cualchi informazion e podarès no rifleti il file modificât.',
'metadata-expand'   => 'Mostre plui detais',
'metadata-collapse' => 'Plate detais',
'metadata-fields'   => 'I cjamps relatîfs ai metadâts EXIF elencâts ca sot a vignaran mostrâts inte pagjine de figure cuant che la tabele dai metadâts e je mostrade inte forme curte. Par impostazion predeterminade, ducj chei altris cjamps a vignaran platâts.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'       => 'Largjece',
'exif-imagelength'      => 'Altece',
'exif-orientation'      => 'Orientament',
'exif-xresolution'      => 'Risoluzion orizontâl',
'exif-yresolution'      => 'Risoluzion verticâl',
'exif-imagedescription' => 'Titul de figure',
'exif-make'             => 'Produtôr machine',
'exif-model'            => 'Model di machine fotografiche',
'exif-software'         => 'Software doprât',
'exif-artist'           => 'Autôr',
'exif-datetimeoriginal' => 'Date e ore di creazion dai dâts',
'exif-exposuretime'     => 'Timp di esposizion',
'exif-flash'            => 'Flash',
'exif-focallength'      => 'Lungjece focâl obietîf',
'exif-contrast'         => 'Control contrast',

# EXIF attributes
'exif-compression-1' => 'Cence compression',

'exif-unknowndate' => 'Date no cognossude',

'exif-orientation-1' => 'Normâl', # 0th row: top; 0th column: left

# External editor support
'edit-externally'      => 'Modifiche chest file cuntune aplicazion esterne',
'edit-externally-help' => 'Cjale [http://www.mediawiki.org/wiki/Manual:External_editors setup instructions] par altris informazions.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'ducj',
'imagelistall'     => 'ducj',
'watchlistall2'    => 'dutis',
'namespacesall'    => 'ducj',
'monthsall'        => 'ducj',

# E-mail address confirmation
'confirmemail'           => 'Conferme direzione di pueste',
'confirmemail_noemail'   => 'No tu âs metût une direzion di pueste eletroniche valide intes tôs [[Special:Preferences|preferencis]].',
'confirmemail_text'      => 'Cheste wiki ti domande di validÂ la to direzion di pueste eletroniche prime di doprâ lis funzions di email. Ative il boton ca sot par inviâ un codiç di conferme ae to direzion. Chest messaç al includarà un leam cuntun codiç; cjame il leam tal to sgarfadôr par confermâ la validitât de tô direzion.',
'confirmemail_send'      => 'Mande un codiç di conferme',
'confirmemail_sent'      => 'Messaç di conferme mandât.',
'confirmemail_needlogin' => 'Al covente $1 par confermâ la to direzion di pueste eletroniche.',
'confirmemail_success'   => 'La tô direzion di pueste e je stade confermade. Tu puedis cumò jentrâ e gjoldi la wiki.',
'confirmemail_loggedin'  => 'La tô direzion di pueste e je stade confermade.',
'confirmemail_subject'   => '{{SITENAME}}: richieste di conferme de direzion di pueste',

# Scary transclusion
'scarytranscludedisabled' => '[Inclusion dai interwikis no ative]',
'scarytranscludefailed'   => '[Recupar dal model falît par $1]',
'scarytranscludetoolong'  => '[URL masse lungje]',

# Delete conflict
'recreate' => 'Torne a creâ',

# HTML dump
'redirectingto' => 'Daûr a tornâ a indreçâ a [[:$1]]...',

# action=purge
'confirm_purge_button' => 'Va indevant',

# AJAX search
'searchcontaining' => "Ricercje des pagjinis che a àn dentri ''$1''.",
'searchnamed'      => "Ricercje des pagjinis cun titul ''$1''.",
'articletitles'    => "Ricercje des pagjinis che a scomencin cun ''$1''",
'hideresults'      => 'Plate risultâts',
'useajaxsearch'    => 'Dopre la ricercje AJAX',

# Multipage image navigation
'imgmultipageprev' => '← pagjine precedente',
'imgmultipagenext' => 'pagjine sucessive →',
'imgmultigo'       => 'Va!',
'imgmultigoto'     => 'Va ae pagjine $1',

# Table pager
'ascending_abbrev'         => 'asc',
'descending_abbrev'        => 'disc',
'table_pager_next'         => 'Pagjine sucessive',
'table_pager_prev'         => 'Pagjine precedente',
'table_pager_first'        => 'Prime pagjine',
'table_pager_last'         => 'Ultime pagjine',
'table_pager_limit'        => 'Mostre $1 elements in ogni pagjine',
'table_pager_limit_submit' => 'Va',

# Auto-summaries
'autosumm-blank' => 'Pagjine disvuedade fûr par fûr',
'autosumm-new'   => 'Gnove pagjine: $1',

# Live preview
'livepreview-loading' => 'Daûr a cjamâ…',
'livepreview-ready'   => 'Daûr a cjamâ… pront!',

# Watchlist editor
'watchlistedit-numitems'      => 'La liste des pagjinis tignudis di voli e conten {{PLURAL:$1|une pagjine|$1 pagjinis}}, cence contâ lis pagjinis di discussion.',
'watchlistedit-noitems'       => 'La liste des pagjinis tignudis di voli e je vueide.',
'watchlistedit-normal-title'  => 'Modifiche tignûts di voli',
'watchlistedit-normal-submit' => 'Elimine pagjinis',
'watchlistedit-normal-done'   => '{{PLURAL:$1|1 pagjine e je stade eliminade|$1 pagjinis a son stadis eliminadis}} de liste des pagjinis tignudis di voli:',
'watchlistedit-raw-titles'    => 'Pagjinis:',

# Watchlist editing tools
'watchlisttools-view' => 'Cjale i cambiaments rilevants',
'watchlisttools-edit' => 'Cjale e cambie la liste des pagjinis tignudis di voli',
'watchlisttools-raw'  => 'Modifiche la liste des pagjinis tignudis di voli in formât testuâl',

# Special:Version
'version'                  => 'Version', # Not used as normal message but as header for the special page itself
'version-version'          => 'Version',
'version-software-version' => 'Version',

# Special:FilePath
'filepath-page' => 'Non dal file:',

# Special:SpecialPages
'specialpages'                 => 'Pagjinis speciâls',
'specialpages-note'            => '----
* Pagjinis speciâls no riservadis.
* <span class="mw-specialpagerestricted">Pagjinis speciâls a ciertis categoriis di utents.</span>',
'specialpages-group-other'     => 'Altris pagjinis speciâls',
'specialpages-group-changes'   => 'Ultins cambiaments e regjistris',
'specialpages-group-users'     => 'Utents e dirits',
'specialpages-group-pages'     => 'Listis di pagjinis',
'specialpages-group-pagetools' => 'Imprescj utii pes pagjinis',

# Special:BlankPage
'blankpage' => 'Pagjine vueide',

);
